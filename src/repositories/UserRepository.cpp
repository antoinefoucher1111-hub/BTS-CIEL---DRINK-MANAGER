// src/repositories/UserRepository.cpp
#include "UserRepository.hpp"
#include <iostream>

UserRepository::UserRepository(sqlite3* database) : db(database) {}

void UserRepository::initTables() {
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fingerprint_id INTEGER UNIQUE,"
        "name TEXT NOT NULL,"
        "email TEXT DEFAULT '',"
        "balance REAL DEFAULT 0.0,"
        "is_admin INTEGER DEFAULT 0);"
        
        "CREATE TABLE IF NOT EXISTS drinks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "price REAL NOT NULL);"
        
        "CREATE TABLE IF NOT EXISTS transactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER,"
        "drink_id INTEGER,"
        "amount REAL,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);"
        
        // Insertion de boissons de test si table vide
        "INSERT OR IGNORE INTO drinks (id, name, price) VALUES "
        "(1, 'Espresso', 0.80), (2, 'Double Espresso', 1.20), (3, 'Allongé', 0.90), "
        "(4, 'Cappuccino', 1.50), (5, 'Thé / Infusion', 0.60);"
        
        // Insertion utilisateur par défaut pour les tests
        "INSERT OR IGNORE INTO users (id, fingerprint_id, name, balance) VALUES "
        "(1, 10, 'Erwann', 10.00);";

    char* errMsg = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (errMsg) {
        std::cerr << "Erreur initTables : " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

std::optional<User> UserRepository::getUserById(int userId) {
    const char* sql = "SELECT id, fingerprint_id, name, email, balance, is_admin FROM users WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, userId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.fingerprintId = sqlite3_column_int(stmt, 1);
        u.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        u.email = email ? email : "";
        u.balance = sqlite3_column_double(stmt, 4);
        u.isAdmin = (sqlite3_column_int(stmt, 5) == 1);
        sqlite3_finalize(stmt);
        return u;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool UserRepository::updateUserProfile(int userId, const std::string& name) {
    const char* sql = "UPDATE users SET name = ? WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool UserRepository::updateBalance(int userId, double delta) {
    const char* sql = "UPDATE users SET balance = balance + ? WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_double(stmt, 1, delta);
    sqlite3_bind_int(stmt, 2, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<Drink> UserRepository::getDrinks() {
    std::vector<Drink> list;
    const char* sql = "SELECT id, name, price FROM drinks;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Drink d;
        d.id = sqlite3_column_int(stmt, 0);
        d.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        d.price = sqlite3_column_double(stmt, 2);
        list.push_back(d);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::optional<Drink> UserRepository::getDrinkById(int drinkId) {
    const char* sql = "SELECT id, name, price FROM drinks WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, drinkId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Drink d;
        d.id = sqlite3_column_int(stmt, 0);
        d.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        d.price = sqlite3_column_double(stmt, 2);
        sqlite3_finalize(stmt);
        return d;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}
