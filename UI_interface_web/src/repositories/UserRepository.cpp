#include "UserRepository.hpp"
#include <iostream>

UserRepository::UserRepository(sqlite3* database) : db(database) {}

std::optional<User> getUserById(int userId) {
    const char* sql = 
        "SELECT u.idUser, u.Name, COALESCE(u.AccessPrint, 0), COALESCE(w.Solde, 0.0) "
        "FROM Users u "
        "LEFT JOIN Wallet w ON u.idUser = w.idUser "
        "WHERE u.idUser = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, userId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        User u;
        u.id = sqlite3_column_int(stmt, 0);
        u.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        u.accessPrint = sqlite3_column_int(stmt, 2);
        u.balance = sqlite3_column_double(stmt, 3);
        sqlite3_finalize(stmt);
        return u;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool UserRepository::updateUserName(int userId, const std::string& newName) {
    const char* sql = "UPDATE Users SET Name = ? WHERE idUser = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool UserRepository::updateBalance(int userId, double delta) {
    const char* sql = 
        "INSERT INTO Wallet (idUser, Solde) VALUES (?, ?) "
        "ON CONFLICT(idUser) DO UPDATE SET Solde = Solde + ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_double(stmt, 2, delta);
    sqlite3_bind_double(stmt, 3, delta);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool UserRepository::recordPurchase(int userId, int drinkId, int quantity, const std::string& description) {
    const char* stockSql = "UPDATE Drinks SET QuantityStock = QuantityStock - ? WHERE idDrink = ?;";
    sqlite3_stmt* stockStmt;
    if (sqlite3_prepare_v2(db, stockSql, -1, &stockStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stockStmt, 1, quantity);
        sqlite3_bind_int(stockStmt, 2, drinkId);
        sqlite3_step(stockStmt);
        sqlite3_finalize(stockStmt);
    }

    const char* histSql = 
        "INSERT INTO Historic (idUser, idDrink, Quantity, datetime, Description) "
        "VALUES (?, ?, ?, datetime('now', 'localtime'), ?);";

    sqlite3_stmt* histStmt;
    if (sqlite3_prepare_v2(db, histSql, -1, &histStmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(histStmt, 1, userId);
    sqlite3_bind_int(histStmt, 2, drinkId);
    sqlite3_bind_int(histStmt, 3, quantity);
    sqlite3_bind_text(histStmt, 4, description.c_str(), -1, SQLITE_STATIC);

    bool ok = (sqlite3_step(histStmt) == SQLITE_DONE);
    sqlite3_finalize(histStmt);
    return ok;
}

std::vector<Drink> UserRepository::getDrinks() {
    std::vector<Drink> list;
    const char* sql = "SELECT idDrink, Name, Category, QuantityStock, Price FROM Drinks WHERE QuantityStock > 0;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Drink d;
        d.id = sqlite3_column_int(stmt, 0);
        d.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        d.category = cat ? cat : "";
        d.stock = sqlite3_column_int(stmt, 3);
        d.price = sqlite3_column_double(stmt, 4);
        list.push_back(d);
    }
    sqlite3_finalize(stmt);
    return list;
}

std::optional<Drink> UserRepository::getDrinkById(int drinkId) {
    const char* sql = "SELECT idDrink, Name, Category, QuantityStock, Price FROM Drinks WHERE idDrink = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_int(stmt, 1, drinkId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Drink d;
        d.id = sqlite3_column_int(stmt, 0);
        d.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* cat = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        d.category = cat ? cat : "";
        d.stock = sqlite3_column_int(stmt, 3);
        d.price = sqlite3_column_double(stmt, 4);
        sqlite3_finalize(stmt);
        return d;
    }
    sqlite3_finalize(stmt);
    return std::nullopt;
}