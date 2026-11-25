#include "httplib.h"
#include "repositories/UserRepository.hpp"
#include "materiel/protocole.h"
#include <sqlite3.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <cctype>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <random>
#include <iomanip>
#include <cstdlib>

// ---------- helpers ----------
static std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    return out;
}

static bool valueStart(const std::string& body, const std::string& key, size_t& pos) {
    auto k = body.find("\"" + key + "\"");
    if (k == std::string::npos) return false;
    auto c = body.find(':', k + key.size() + 2);
    if (c == std::string::npos) return false;
    pos = c + 1;
    while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) ++pos;
    return pos < body.size();
}

static bool getInt(const std::string& body, const std::string& key, int& out) {
    size_t pos;
    if (!valueStart(body, key, pos)) return false;
    try { out = std::stoi(body.substr(pos)); return true; } catch (...) { return false; }
}

static bool getDouble(const std::string& body, const std::string& key, double& out) {
    size_t pos;
    if (!valueStart(body, key, pos)) return false;
    try { out = std::stod(body.substr(pos)); return true; } catch (...) { return false; }
}

static bool getString(const std::string& body, const std::string& key, std::string& out) {
    size_t pos;
    if (!valueStart(body, key, pos) || body[pos] != '"') return false;
    auto end = body.find('"', pos + 1);
    if (end == std::string::npos) return false;
    out = body.substr(pos + 1, end - pos - 1);
    return true;
}

static void sendError(httplib::Response& res, int status, const std::string& msg) {
    res.status = status;
    res.set_content("{\"error\":\"" + jsonEscape(msg) + "\"}", "application/json");
}

static std::string userToJson(const User& u) {
    std::ostringstream json;
    json << "{\"id\":" << u.id
         << ",\"name\":\"" << jsonEscape(u.name)
         << "\",\"balance\":" << u.balance
         << ",\"access_print\":" << u.accessPrint << "}";
    return json.str();
}

static std::string resolveDbPath() {
    const std::string candidates[] = {
        "data/drinkmanager.db",
        "backend/data/drinkmanager.db",
        "../data/drinkmanager.db",
        "C:/Users/antoi/Documents/Ui_Interface_web/backend/data/drinkmanager.db"
    };
    for (const auto& p : candidates) {
        if (std::filesystem::exists(p)) return p;
    }
    return "C:/Users/antoi/Documents/Ui_Interface_web/backend/data/drinkmanager.db";
}

struct PendingAuthCode {
    int userId;
    std::chrono::steady_clock::time_point expiresAt;
};

static std::string generate6DigitCode() {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 999999);
    std::ostringstream oss;
    oss << std::setw(6) << std::setfill('0') << dist(rng);
    return oss.str();
}

struct Session {
    int userId;
    std::chrono::steady_clock::time_point expiresAt;
};

static std::string generateToken() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << rng()
        << std::setw(16) << std::setfill('0') << rng();
    return oss.str();
}

static bool isLocalRequest(const httplib::Request& req) {
    return req.remote_addr == "127.0.0.1" || req.remote_addr == "::1" || req.remote_addr == "localhost";
}

// ---------- main ----------
int main() {
    // DM_ALLOW_SIM=1 : autorise le simulateur de capteur depuis le navigateur (tests uniquement)
    const char* simEnv = std::getenv("DM_ALLOW_SIM");
    const bool allowSim = simEnv && std::string(simEnv) == "1";
    if (allowSim) std::cout << "[WARN] Simulateur de capteur autorise (DM_ALLOW_SIM=1)\n";

    sqlite3* db = nullptr;
    std::string dbPath = resolveDbPath();

    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "[ERREUR] Impossible d'ouvrir " << dbPath << "\n";
        return 1;
    }
    std::cout << "[INFO] Base " << dbPath << " ouverte avec succes.\n";

    UserRepository repo(db);
    std::mutex dbMutex;
    std::mutex authMutex;
    std::unordered_map<std::string, PendingAuthCode> activeCodes;
    std::unordered_map<std::string, Session> sessions;

    // Retourne l'id utilisateur du jeton "Authorization: Bearer <token>", ou -1
    auto authUserId = [&](const httplib::Request& req) -> int {
        std::string h = req.get_header_value("Authorization");
        const std::string prefix = "Bearer ";
        if (h.rfind(prefix, 0) == 0) h = h.substr(prefix.size());
        if (h.empty()) return -1;
        std::lock_guard<std::mutex> lk(authMutex);
        auto it = sessions.find(h);
        if (it == sessions.end()) return -1;
        if (it->second.expiresAt < std::chrono::steady_clock::now()) { sessions.erase(it); return -1; }
        return it->second.userId;
    };
    // true si authentifie ; si expectedId >= 0 le jeton doit appartenir a ce compte
    auto requireAuth = [&](const httplib::Request& req, httplib::Response& res, int expectedId) -> bool {
        int uid = authUserId(req);
        if (uid < 0) { sendError(res, 401, "Authentification requise (code biometrique)"); return false; }
        if (expectedId >= 0 && uid != expectedId) { sendError(res, 403, "Acces refuse a ce compte"); return false; }
        return true;
    };
    auto requireLocal = [&](const httplib::Request& req, httplib::Response& res) -> bool {
        if (!isLocalRequest(req)) { sendError(res, 403, "Reserve a la borne (acces local)"); return false; }
        return true;
    };

    httplib::Server svr;

    svr.set_post_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    svr.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // 0. GET /api/health
    svr.Get("/api/health", [&dbPath](const httplib::Request&, httplib::Response& res) {
        std::ostringstream json;
        json << "{\"status\":\"ok\",\"backend\":\"C++17 httplib\",\"database\":\""
             << jsonEscape(dbPath) << "\"}";
        res.set_content(json.str(), "application/json");
    });

    // 1. GET /api/users
    svr.Get("/api/users", [&](const httplib::Request& req, httplib::Response& res) {
        if (!requireLocal(req, res)) return;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto users = repo.getAllUsers();
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < users.size(); ++i) {
            json << userToJson(users[i]) << (i + 1 < users.size() ? "," : "");
        }
        json << "]";
        res.set_content(json.str(), "application/json");
    });

    // 2. POST /api/users
    svr.Post("/api/users", [&](const httplib::Request& req, httplib::Response& res) {
        if (!requireLocal(req, res)) return;
        std::string name;
        if (!getString(req.body, "name", name) || name.empty()) {
            sendError(res, 400, "Nom manquant");
            return;
        }
        double initialBalance = 0.0;
        getDouble(req.body, "balance", initialBalance);
        if (initialBalance < 0.0) initialBalance = 0.0;

        int accessPrint = 0;
        getInt(req.body, "access_print", accessPrint);
        if (accessPrint < 0 || accessPrint > Protocole::ID_MAX) accessPrint = 0;

        std::lock_guard<std::mutex> lock(dbMutex);
        auto created = repo.createUser(name, initialBalance, accessPrint);
        if (!created) {
            sendError(res, 500, "Impossible de creer l'utilisateur");
            return;
        }
        res.status = 201;
        res.set_content(userToJson(*created), "application/json");
    });

    // 3. GET /api/users/:id
    svr.Get(R"(/api/users/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int userId;
        try { userId = std::stoi(req.matches[1]); } catch (...) { sendError(res, 400, "Id invalide"); return; }
        if (!requireAuth(req, res, userId)) return;

        std::lock_guard<std::mutex> lock(dbMutex);
        auto user = repo.getUserById(userId);
        if (!user) { sendError(res, 404, "Utilisateur introuvable"); return; }

        res.set_content(userToJson(*user), "application/json");
    });

    // 4. PUT /api/users/:id
    svr.Put(R"(/api/users/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int userId;
        try { userId = std::stoi(req.matches[1]); } catch (...) { sendError(res, 400, "Id invalide"); return; }
        if (!requireAuth(req, res, userId)) return;

        std::string newName;
        if (!getString(req.body, "name", newName) || newName.empty()) {
            sendError(res, 400, "Nom manquant");
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        if (!repo.updateUserName(userId, newName)) { sendError(res, 404, "Utilisateur introuvable ou echec mise a jour"); return; }
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    // 4b. PUT /api/users/:id/fingerprint
    svr.Put(R"(/api/users/(\d+)/fingerprint)", [&](const httplib::Request& req, httplib::Response& res) {
        if (!requireLocal(req, res)) return;
        int userId;
        try { userId = std::stoi(req.matches[1]); } catch (...) { sendError(res, 400, "Id invalide"); return; }

        int accessPrint = 0;
        if (!getInt(req.body, "access_print", accessPrint) || accessPrint < 0 || accessPrint > Protocole::ID_MAX) {
            sendError(res, 400, "access_print invalide (0..127)");
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        if (!repo.updateUserAccessPrint(userId, accessPrint)) {
            sendError(res, 404, "Utilisateur introuvable");
            return;
        }
        std::ostringstream json;
        json << "{\"status\":\"success\",\"enroll_command\":\""
             << jsonEscape(Protocole::commandeEnroll(accessPrint)) << "\"}";
        res.set_content(json.str(), "application/json");
    });

    // 5. POST /api/auth/biometric (Pont capteur AS608 / Arduino / Raspberry Pi -> Code 6 chiffres)
    svr.Post("/api/auth/biometric", [&](const httplib::Request& req, httplib::Response& res) {
        if (!allowSim && !requireLocal(req, res)) return;
        int fingerprintId = 0;
        std::string serialLine;
        if (getString(req.body, "serial_line", serialLine) && !serialLine.empty()) {
            auto msg = Protocole::analyser(serialLine);
            if (msg.type != Protocole::Type::UserOk) {
                sendError(res, 401, "Authentification biometrique refusee (" + serialLine + ")");
                return;
            }
            fingerprintId = msg.id;
        } else if (!getInt(req.body, "fingerprint_id", fingerprintId)) {
            sendError(res, 400, "fingerprint_id ou serial_line manquant");
            return;
        }

        if (fingerprintId < 1 || fingerprintId > Protocole::ID_MAX) {
            sendError(res, 400, "ID empreinte invalide (1..127)");
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        auto user = repo.getUserByAccessPrint(fingerprintId);
        if (!user) {
            sendError(res, 404, "Aucun utilisateur associe a cette empreinte");
            return;
        }

        std::string code = generate6DigitCode();
        {
            std::lock_guard<std::mutex> aLock(authMutex);
            auto now = std::chrono::steady_clock::now();
            for (auto it = activeCodes.begin(); it != activeCodes.end(); ) {
                if (it->second.expiresAt < now) it = activeCodes.erase(it);
                else ++it;
            }
            activeCodes[code] = PendingAuthCode{user->id, now + std::chrono::seconds(120)};
        }

        std::ostringstream json;
        json << "{\"status\":\"ok\",\"code\":\"" << code
             << "\",\"serial_command\":\"" << jsonEscape(Protocole::commandeCode(code))
             << "\",\"user\":" << userToJson(*user) << "}";
        res.set_content(json.str(), "application/json");
    });

    // 5b. POST /api/auth/verify-code (Connexion web par code a 6 chiffres)
    svr.Post("/api/auth/verify-code", [&](const httplib::Request& req, httplib::Response& res) {
        std::string code;
        if (!getString(req.body, "code", code) || !Protocole::codeValide(code)) {
            sendError(res, 400, "Code invalide (6 chiffres requis)");
            return;
        }

        int matchedUserId = -1;
        {
            std::lock_guard<std::mutex> aLock(authMutex);
            auto now = std::chrono::steady_clock::now();
            auto it = activeCodes.find(code);
            if (it != activeCodes.end()) {
                if (it->second.expiresAt >= now) {
                    matchedUserId = it->second.userId;
                }
                activeCodes.erase(it);
            }
        }

        if (matchedUserId < 0) {
            sendError(res, 401, "Code inconnu ou expire");
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        auto user = repo.getUserById(matchedUserId);
        if (!user) {
            sendError(res, 404, "Utilisateur introuvable");
            return;
        }

        std::string token = generateToken();
        {
            std::lock_guard<std::mutex> aLock(authMutex);
            auto now = std::chrono::steady_clock::now();
            for (auto it = sessions.begin(); it != sessions.end(); ) {
                if (it->second.expiresAt < now) it = sessions.erase(it);
                else ++it;
            }
            sessions[token] = Session{user->id, now + std::chrono::hours(8)};
        }
        std::string json = userToJson(*user);
        json.pop_back();
        json += ",\"token\":\"" + token + "\"}";
        res.set_content(json, "application/json");
    });

    // 6. GET /api/users/:id/history
    svr.Get(R"(/api/users/(\d+)/history)", [&](const httplib::Request& req, httplib::Response& res) {
        int userId;
        try { userId = std::stoi(req.matches[1]); } catch (...) { sendError(res, 400, "Id invalide"); return; }
        if (!requireAuth(req, res, userId)) return;

        std::lock_guard<std::mutex> lock(dbMutex);
        if (!repo.getUserById(userId)) { sendError(res, 404, "Utilisateur introuvable"); return; }
        auto history = repo.getUserHistory(userId);

        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < history.size(); ++i) {
            json << "{\"id\":" << history[i].id
                 << ",\"user_id\":" << history[i].userId
                 << ",\"drink_id\":" << history[i].drinkId
                 << ",\"drink_name\":\"" << jsonEscape(history[i].drinkName) << "\""
                 << ",\"quantity\":" << history[i].quantity
                 << ",\"datetime\":\"" << jsonEscape(history[i].datetime) << "\""
                 << ",\"description\":\"" << jsonEscape(history[i].description) << "\"}"
                 << (i + 1 < history.size() ? "," : "");
        }
        json << "]";
        res.set_content(json.str(), "application/json");
    });

    // 7. GET /api/history/xml
    svr.Get("/api/history/xml", [&](const httplib::Request& req, httplib::Response& res) {
        if (!requireAuth(req, res, -1)) return;
        std::optional<int> userId = authUserId(req);  // toujours limite au compte connecte
        std::lock_guard<std::mutex> lock(dbMutex);
        std::string xml = repo.exportHistoryToXML(userId);
        res.set_header("Content-Disposition", "attachment; filename=\"historique.xml\"");
        res.set_content(xml, "application/xml; charset=utf-8");
    });

    // 8. GET /api/drinks
    svr.Get("/api/drinks", [&](const httplib::Request& req, httplib::Response& res) {
        if (!requireAuth(req, res, -1)) return;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto drinks = repo.getDrinks();

        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < drinks.size(); ++i) {
            json << "{\"id\":" << drinks[i].id
                 << ",\"name\":\"" << jsonEscape(drinks[i].name)
                 << "\",\"category\":\"" << jsonEscape(drinks[i].category)
                 << "\",\"price\":" << drinks[i].price
                 << ",\"stock\":" << drinks[i].stock << "}"
                 << (i + 1 < drinks.size() ? "," : "");
        }
        json << "]";
        res.set_content(json.str(), "application/json");
    });

    // 9. POST /api/orders
    svr.Post("/api/orders", [&](const httplib::Request& req, httplib::Response& res) {
        int userId = 0, drinkId = 0, quantity = 1;
        if (!getInt(req.body, "user_id", userId) || !getInt(req.body, "drink_id", drinkId)) {
            sendError(res, 400, "user_id ou drink_id manquant");
            return;
        }
        if (!requireAuth(req, res, userId)) return;
        getInt(req.body, "quantity", quantity);
        if (quantity <= 0) {
            sendError(res, 400, "Quantite invalide");
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        std::string errorMsg;
        double newBalance = 0.0;
        if (!repo.processOrder(userId, drinkId, quantity, errorMsg, newBalance)) {
            int code = (errorMsg.find("introuvable") != std::string::npos) ? 404 : 400;
            sendError(res, code, errorMsg);
            return;
        }

        auto updatedDrink = repo.getDrinkById(drinkId);
        int newStock = updatedDrink ? updatedDrink->stock : 0;

        std::ostringstream json;
        json << "{\"status\":\"success\",\"new_balance\":" << newBalance
             << ",\"new_stock\":" << newStock << "}";
        res.set_content(json.str(), "application/json");
    });

    // 10. POST /api/recharge
    svr.Post("/api/recharge", [&](const httplib::Request& req, httplib::Response& res) {
        int userId = 0; double amount = 0.0;
        if (!getInt(req.body, "user_id", userId) || !getDouble(req.body, "amount", amount)) {
            sendError(res, 400, "user_id ou amount manquant");
            return;
        }
        if (!requireAuth(req, res, userId)) return;
        if (amount <= 0) { sendError(res, 400, "Montant invalide"); return; }

        std::lock_guard<std::mutex> lock(dbMutex);
        if (!repo.getUserById(userId)) { sendError(res, 404, "Utilisateur introuvable"); return; }

        if (!repo.updateBalance(userId, amount)) {
            sendError(res, 500, "Echec du rechargement");
            return;
        }
        auto updated = repo.getUserById(userId);
        if (!updated) { sendError(res, 500, "Erreur interne"); return; }

        std::ostringstream json;
        json << "{\"status\":\"success\",\"new_balance\":" << updated->balance << "}";
        res.set_content(json.str(), "application/json");
    });

    std::cout << "[INFO] Serveur actif sur http://0.0.0.0:8080\n";
    svr.listen("0.0.0.0", 8080);

    sqlite3_close(db);
    return 0;
}
