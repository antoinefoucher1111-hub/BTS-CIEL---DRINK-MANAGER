#include "httplib.h"
#include "repositories/UserRepository.hpp"
#include <sqlite3.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <cctype>

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

// ---------- main ----------
int main() {
    sqlite3* db;
    
    // CORRECTION : Utilisation du chemin absolu pour garantir que la base est trouvée
    const char* dbPath = "C:/Users/antoi/Documents/Ui_Interface_web/backend/data/drinkmanager.db";
    
    if (sqlite3_open(dbPath, &db) != SQLITE_OK) {
        std::cerr << "[ERREUR] Impossible d'ouvrir " << dbPath
                  << " (Vérifie que le dossier existe !)\n";
        return 1;
    }
    std::cout << "[INFO] Base " << dbPath << " ouverte avec succes.\n";

    UserRepository repo(db);
    std::mutex dbMutex;
    httplib::Server svr;

    svr.set_post_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    svr.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // 1. GET /api/users/:id
    svr.Get(R"(/api/users/(\d+))", [&repo, &dbMutex](const httplib::Request& req, httplib::Response& res) {
        int userId;
        try { userId = std::stoi(req.matches[1]); } catch (...) { sendError(res, 400, "Id invalide"); return; }

        std::lock_guard<std::mutex> lock(dbMutex);
        auto user = repo.getUserById(userId);
        if (!user) { sendError(res, 404, "Utilisateur introuvable"); return; }

        std::ostringstream json;
        json << "{\"id\":" << user->id << ",\"name\":\"" << jsonEscape(user->name)
             << "\",\"balance\":" << user->balance << "}";
        res.set_content(json.str(), "application/json");
    });

    // 2. PUT /api/users/:id
    svr.Put(R"(/api/users/(\d+))", [&repo, &dbMutex](const httplib::Request& req, httplib::Response& res) {
        int userId;
        try { userId = std::stoi(req.matches[1]); } catch (...) { sendError(res, 400, "Id invalide"); return; }

        std::string newName;
        if (!getString(req.body, "name", newName) || newName.empty()) {
            sendError(res, 400, "Nom manquant");
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        if (!repo.updateUserName(userId, newName)) { sendError(res, 500, "Echec mise a jour"); return; }
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    // 3. GET /api/drinks
    svr.Get("/api/drinks", [&repo, &dbMutex](const httplib::Request&, httplib::Response& res) {
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

    // 4. POST /api/orders
    svr.Post("/api/orders", [&repo, &dbMutex](const httplib::Request& req, httplib::Response& res) {
        int userId = 0, drinkId = 0;
        if (!getInt(req.body, "user_id", userId) || !getInt(req.body, "drink_id", drinkId)) {
            sendError(res, 400, "user_id ou drink_id manquant");
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        auto user = repo.getUserById(userId);
        auto drink = repo.getDrinkById(drinkId);
        if (!user || !drink) { sendError(res, 404, "Utilisateur ou boisson introuvable"); return; }
        if (user->balance < drink->price) { sendError(res, 400, "Solde insuffisant"); return; }

        repo.updateBalance(userId, -drink->price);
        repo.recordPurchase(userId, drinkId, 1, "Achat de 1 " + drink->name);
        auto updated = repo.getUserById(userId);
        if (!updated) { sendError(res, 500, "Erreur interne"); return; }

        std::ostringstream json;
        json << "{\"status\":\"success\",\"new_balance\":" << updated->balance << "}";
        res.set_content(json.str(), "application/json");
    });

    // 5. POST /api/recharge
    svr.Post("/api/recharge", [&repo, &dbMutex](const httplib::Request& req, httplib::Response& res) {
        int userId = 0; double amount = 0.0;
        if (!getInt(req.body, "user_id", userId) || !getDouble(req.body, "amount", amount)) {
            sendError(res, 400, "user_id ou amount manquant");
            return;
        }
        if (amount <= 0) { sendError(res, 400, "Montant invalide"); return; }

        std::lock_guard<std::mutex> lock(dbMutex);
        if (!repo.getUserById(userId)) { sendError(res, 404, "Utilisateur introuvable"); return; }

        repo.updateBalance(userId, amount);
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