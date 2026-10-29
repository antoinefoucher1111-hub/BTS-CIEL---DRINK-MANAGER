#include "UserRepository.hpp"
#include <iostream>

// Constructeur
UserRepository::UserRepository(sqlite3* db) : db(db) {}

// Récupérer un utilisateur et son solde
std::optional<User> UserRepository::getUserById(int id) {
    std::string sql = "SELECT U.idUser, U.Name, W.Solde FROM USERS U JOIN WALLET W ON U.idUser = W.idUser WHERE U.idUser = " + std::to_string(id) + ";";
    sqlite3_stmt* stmt;
    std::optional<User> user;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user = User{
                sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                sqlite3_column_double(stmt, 2)
            };
        }
    }
    sqlite3_finalize(stmt);
    return user;
}

// Mettre à jour le nom
bool UserRepository::updateUserName(int id, const std::string& name) {
    std::string sql = "UPDATE USERS SET Name = '" + name + "' WHERE idUser = " + std::to_string(id) + ";";
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}

// Mettre à jour le solde (Rechargement)
bool UserRepository::updateBalance(int id, double amount) {
    std::string sql = "UPDATE WALLET SET Solde = Solde + " + std::to_string(amount) + " WHERE idUser = " + std::to_string(id) + ";";
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}

// Enregistrer un achat (Stock -1 et Historique)
bool UserRepository::recordPurchase(int userId, int drinkId, int quantity, const std::string& description) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    
    std::string sql1 = "UPDATE DRINKS SET QuantityStock = QuantityStock - " + std::to_string(quantity) + " WHERE idDrink = " + std::to_string(drinkId) + ";";
    std::string sql2 = "INSERT INTO HISTORIC (idUser, idDrink, Quantity, Description) VALUES (" + 
                       std::to_string(userId) + ", " + std::to_string(drinkId) + ", " + std::to_string(quantity) + ", '" + description + "');";

    if (sqlite3_exec(db, sql1.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK &&
        sqlite3_exec(db, sql2.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK) {
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
        return true;
    } else {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
}

// Récupérer toutes les boissons
std::vector<Drink> UserRepository::getDrinks() {
    std::vector<Drink> drinks;
    const char* sql = "SELECT idDrink, Name, Category, QuantityStock, Price FROM DRINKS;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            drinks.push_back({
                sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
                sqlite3_column_int(stmt, 3),
                sqlite3_column_double(stmt, 4)
            });
        }
    }
    sqlite3_finalize(stmt);
    return drinks;
}

// Récupérer une boisson par son ID
std::optional<Drink> UserRepository::getDrinkById(int id) {
    std::string sql = "SELECT idDrink, Name, Category, QuantityStock, Price FROM DRINKS WHERE idDrink = " + std::to_string(id) + ";";
    sqlite3_stmt* stmt;
    std::optional<Drink> drink;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            drink = Drink{
                sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
                sqlite3_column_int(stmt, 3),
                sqlite3_column_double(stmt, 4)
            };
        }
    }
    sqlite3_finalize(stmt);
    return drink;
}