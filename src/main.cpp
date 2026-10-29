// src/main.cpp
#include "httplib.h"
#include "repositories/UserRepository.hpp"
#include <sqlite3.h>
#include <iostream>
#include <mutex>
#include <sstream>

int main() {
    sqlite3* db;
    if (sqlite3_open("cafe.db", &db) != SQLITE_OK) {
        std::cerr << "Erreur d'ouverture SQLite\n";
        return 1;
    }

    UserRepository repo(db);
    repo.initTables();

    std::mutex dbMutex;
    httplib::Server svr;

    // Gestion universelle des CORS pour Expo Web
    svr.set_post_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });

    // Requêtes Preflight OPTIONS
    svr.Options(R"(/api/.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // 1. GET /api/users/:id
    svr.Get(R"(/api/users/(\d+))", [&repo, &dbMutex](const httplib::Request& req, httplib::Response& res) {
        int userId = std::stoi(req.matches[1]);

        std::lock_guard<std::mutex> lock(dbMutex);
        auto user = repo.getUserById(userId);

        if (!user) {
            res.status = 404;
            res.set_content("{\"error\": \"Utilisateur non trouvé\"}", "application/json");
            return;
        }

        std::ostringstream json;
        json << "{"
             << "\"id\":" << user->id << ","
             << "\"name\":\"" << user->name << "\","
             << "\"email\":\"" << user->email << "\","
             << "\"balance\":" << user->balance
             << "}";

        res.set_content(json.str(), "application/json");
    });

    // 2. PUT /api/users/:id (Mise à jour profil)
    svr.Put(R"(/api/users/(\d+))", [&repo, &dbMutex](const httplib::Request& req, httplib::Response& res) {
        int userId = std::stoi(req.matches[1]);

        // Extraction simple de la chaîne "name" dans le JSON reçu
        std::string body = req.body;
        std::string key = "\"name\":\"";
        auto pos = body.find(key);
        if (pos == std::string::npos) {
            res.status = 400;
            res.set_content("{\"error\": \"Champ 'name' obligatoire\"}", "application/json");
            return;
        }

        auto start = pos + key.length();
        auto end = body.find("\"", start);
        std::string newName = body.substr(start, end - start);

        std::lock_guard<std::mutex> lock(dbMutex);
        if (!repo.updateUserProfile(userId, newName)) {
            res.status = 500;
            res.set_content("{\"error\": \"Erreur lors de la mise à jour\"}", "application/json");
            return;
        }

        res.set_content("{\"status\": \"success\"}", "application/json");
    });

    // 3. GET /api/drinks
    svr.Get("/api/drinks", [&repo, &dbMutex](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(dbMutex);
        auto drinks = repo.getDrinks();

        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < drinks.size(); ++i) {
            json << "{"
                 << "\"id\":" << drinks[i].id << ","
                 << "\"name\":\"" << drinks[i].name << "\","
                 << "\"price\":" << drinks[i].price
                 << "}" << (i + 1 < drinks.size() ? "," : "");
        }
        json << "]";

        res.set_content(json.str(), "application/json");
    });

    // 4. POST /api/orders
    svr.Post("/api/orders", [&repo, &dbMutex](const httplib::Request& req, httplib::Response& res) {
        // Extraction basique des IDs
        int userId = 1, drinkId = 1;
        try {
            auto posU = req.body.find("\"user_id\":");
            if (posU != std::string::npos) userId = std::stoi(req.body.substr(posU + 10));

            auto posD = req.body.find("\"drink_id\":");
            if (posD != std::string::npos) drinkId = std::stoi(req.body.substr(posD + 11));
        } catch (...) {
            res.status = 400;
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        auto user = repo.getUserById(userId);
        auto drink = repo.getDrinkById(drinkId);

        if (!user || !drink) {
            res.status = 404;
            return;
        }
        if (user->balance < drink->price) {
            res.status = 400;
            res.set_content("{\"error\": \"Solde insuffisant\"}", "application/json");
            return;
        }

        repo.updateBalance(userId, -drink->price);
        auto updated = repo.getUserById(userId);

        std::ostringstream json;
        json << "{\"status\":\"success\",\"new_balance\":" << updated->balance << "}";
        res.set_content(json.str(), "application/json");
    });

    // 5. POST /api/recharge
    svr.Post("/api/recharge", [&repo, &dbMutex](const httplib::Request& req, httplib::Response& res) {
        int userId = 1;
        double amount = 0.0;
        try {
            auto posU = req.body.find("\"user_id\":");
            if (posU != std::string::npos) userId = std::stoi(req.body.substr(posU + 10));

            auto posA = req.body.find("\"amount\":");
            if (posA != std::string::npos) amount = std::stod(req.body.substr(posA + 9));
        } catch (...) {
            res.status = 400;
            return;
        }

        std::lock_guard<std::mutex> lock(dbMutex);
        repo.updateBalance(userId, amount);
        auto updated = repo.getUserById(userId);

        std::ostringstream json;
        json << "{\"status\":\"success\",\"new_balance\":" << updated->balance << "}";
        res.set_content(json.str(), "application/json");
    });

    std::cout << "[INFO] Serveur C++ actif sur http://0.0.0.0:8080\n";
    svr.listen("0.0.0.0", 8080);

    sqlite3_close(db);
    return 0;
}
