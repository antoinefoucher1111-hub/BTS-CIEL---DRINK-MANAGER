#include "Database.h"
#include <iostream>
#include <sstream>

DatabaseManager::DatabaseManager(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db)) {
        std::cerr << "Erreur d'ouverture DB: " << sqlite3_errmsg(db) << std::endl;
    }
}

DatabaseManager::~DatabaseManager() {
    sqlite3_close(db);
}

void DatabaseManager::initializeTables() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS USERS (
            idUser INTEGER PRIMARY KEY AUTOINCREMENT,
            Name TEXT,
            AccessPrint INTEGER
        );
        CREATE TABLE IF NOT EXISTS WALLET (
            idUser INTEGER PRIMARY KEY,
            Solde NUMERIC,
            FOREIGN KEY(idUser) REFERENCES USERS(idUser)
        );
        CREATE TABLE IF NOT EXISTS DRINKS (
            idDrink INTEGER PRIMARY KEY AUTOINCREMENT,
            Name TEXT,
            Category TEXT,
            QuantityStock INTEGER,
            Price NUMERIC
        );
        CREATE TABLE IF NOT EXISTS HISTORIC (
            idHistoric INTEGER PRIMARY KEY AUTOINCREMENT,
            idUser INTEGER,
            idDrink INTEGER,
            Quantity INTEGER,
            datetime DATETIME DEFAULT CURRENT_TIMESTAMP,
            Description TEXT,
            FOREIGN KEY(idUser) REFERENCES USERS(idUser),
            FOREIGN KEY(idDrink) REFERENCES DRINKS(idDrink)
        );
    )";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

void DatabaseManager::seedData() {
    const char* sql = R"(
        INSERT OR IGNORE INTO USERS (idUser, Name, AccessPrint) VALUES (1, 'Erwann', 12345);
        INSERT OR IGNORE INTO WALLET (idUser, Solde) VALUES (1, 8.4);
        INSERT OR IGNORE INTO DRINKS (idDrink, Name, Category, QuantityStock, Price) VALUES (1, 'Espresso', 'Coffee', 10, 0.8);
        INSERT OR IGNORE INTO DRINKS (idDrink, Name, Category, QuantityStock, Price) VALUES (2, 'Thé Vert', 'Tea', 5, 1.2);
    )";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

UserProfile DatabaseManager::getUserProfile(int userId) {
    UserProfile user = {-1, "", 0.0};
    std::string sql = "SELECT U.idUser, U.Name, W.Solde FROM USERS U JOIN WALLET W ON U.idUser = W.idUser WHERE U.idUser = " + std::to_string(userId) + ";";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user.id = sqlite3_column_int(stmt, 0);
            user.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user.balance = sqlite3_column_double(stmt, 2);
        }
    }
    sqlite3_finalize(stmt);
    return user;
}

std::vector<Drink> DatabaseManager::getAllDrinks() {
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

bool DatabaseManager::updateUserName(int userId, const std::string& newName) {
    std::string sql = "UPDATE USERS SET Name = '" + newName + "' WHERE idUser = " + std::to_string(userId) + ";";
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool DatabaseManager::updateUserBalance(int userId, double amountToAdd) {
    std::string sql = "UPDATE WALLET SET Solde = Solde + " + std::to_string(amountToAdd) + " WHERE idUser = " + std::to_string(userId) + ";";
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool DatabaseManager::processOrder(int userId, int drinkId, double price) {
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    std::string sql1 = "UPDATE WALLET SET Solde = Solde - " + std::to_string(price) + " WHERE idUser = " + std::to_string(userId) + ";";
    std::string sql2 = "UPDATE DRINKS SET QuantityStock = QuantityStock - 1 WHERE idDrink = " + std::to_string(drinkId) + ";";
    std::string sql3 = "INSERT INTO HISTORIC (idUser, idDrink, Quantity, Description) VALUES (" + std::to_string(userId) + ", " + std::to_string(drinkId) + ", 1, 'Achat Web');";

    if (sqlite3_exec(db, sql1.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK &&
        sqlite3_exec(db, sql2.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK &&
        sqlite3_exec(db, sql3.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK) {
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
        return true;
    } else {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
}

std::string DatabaseManager::exportHistoricToXML() {
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<historics>\n";
    const char* sql = "SELECT idHistoric, idUser, idDrink, Quantity, datetime, Description FROM HISTORIC;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            xml << "  <order>\n"
                << "    <id>" << sqlite3_column_int(stmt, 0) << "</id>\n"
                << "    <idUser>" << sqlite3_column_int(stmt, 1) << "</idUser>\n"
                << "    <idDrink>" << sqlite3_column_int(stmt, 2) << "</idDrink>\n"
                << "    <quantity>" << sqlite3_column_int(stmt, 3) << "</quantity>\n"
                << "    <datetime>" << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) << "</datetime>\n"
                << "  </order>\n";
        }
    }
    sqlite3_finalize(stmt);
    xml << "</historics>\n";
    return xml.str();
}