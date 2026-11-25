#include "UserRepository.hpp"
#include <iostream>
#include <sstream>

static std::string colText(sqlite3_stmt* stmt, int col) {
    const unsigned char* txt = sqlite3_column_text(stmt, col);
    return txt ? reinterpret_cast<const char*>(txt) : "";
}

static std::string xmlEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;"; break;
            case '<':  out += "&lt;"; break;
            case '>':  out += "&gt;"; break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += c;
        }
    }
    return out;
}

UserRepository::UserRepository(sqlite3* db) : db(db) {}

std::vector<User> UserRepository::getAllUsers() {
    std::vector<User> users;
    const char* sql =
        "SELECT U.idUser, U.Name, COALESCE(W.Solde, 0.0), COALESCE(U.AccessPrint, 0) "
        "FROM Users U LEFT JOIN Wallet W ON U.idUser = W.idUser "
        "ORDER BY U.idUser ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            users.push_back(User{
                sqlite3_column_int(stmt, 0),
                colText(stmt, 1),
                sqlite3_column_double(stmt, 2),
                sqlite3_column_int(stmt, 3)
            });
        }
    }
    sqlite3_finalize(stmt);
    return users;
}

std::optional<User> UserRepository::getUserById(int id) {
    const char* sql =
        "SELECT U.idUser, U.Name, COALESCE(W.Solde, 0.0), COALESCE(U.AccessPrint, 0) "
        "FROM Users U LEFT JOIN Wallet W ON U.idUser = W.idUser "
        "WHERE U.idUser = ?;";
    sqlite3_stmt* stmt = nullptr;
    std::optional<User> user;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user = User{
                sqlite3_column_int(stmt, 0),
                colText(stmt, 1),
                sqlite3_column_double(stmt, 2),
                sqlite3_column_int(stmt, 3)
            };
        }
    }
    sqlite3_finalize(stmt);
    return user;
}

std::optional<User> UserRepository::getUserByAccessPrint(int accessPrint) {
    const char* sql =
        "SELECT U.idUser, U.Name, COALESCE(W.Solde, 0.0), COALESCE(U.AccessPrint, 0) "
        "FROM Users U LEFT JOIN Wallet W ON U.idUser = W.idUser "
        "WHERE U.AccessPrint = ? "
        "ORDER BY U.idUser ASC LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    std::optional<User> user;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, accessPrint);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user = User{
                sqlite3_column_int(stmt, 0),
                colText(stmt, 1),
                sqlite3_column_double(stmt, 2),
                sqlite3_column_int(stmt, 3)
            };
        }
    }
    sqlite3_finalize(stmt);
    return user;
}

std::optional<User> UserRepository::createUser(const std::string& name, double initialBalance, int accessPrint) {
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    const char* insertUserSql = "INSERT INTO Users (Name, AccessPrint) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insertUserSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, accessPrint);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::nullopt;
    }

    int newId = static_cast<int>(sqlite3_last_insert_rowid(db));

    const char* insertWalletSql = "INSERT INTO Wallet (idUser, Solde) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, insertWalletSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::nullopt;
    }
    sqlite3_bind_int(stmt, 1, newId);
    sqlite3_bind_double(stmt, 2, initialBalance);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::nullopt;
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    return User{newId, name, initialBalance, accessPrint};
}

bool UserRepository::updateUserName(int id, const std::string& name) {
    const char* sql = "UPDATE Users SET Name = ? WHERE idUser = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
    int rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE && changes > 0;
}

bool UserRepository::updateUserAccessPrint(int id, int accessPrint) {
    const char* sql = "UPDATE Users SET AccessPrint = ? WHERE idUser = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, accessPrint);
    sqlite3_bind_int(stmt, 2, id);
    int rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE && changes > 0;
}

bool UserRepository::updateBalance(int id, double amount) {
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        return false;
    }

    // Assure que l'utilisateur a bien une ligne dans Wallet
    const char* ensureSql = "INSERT OR IGNORE INTO Wallet (idUser, Solde) VALUES (?, 0.0);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, ensureSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    const char* updateSql = "UPDATE Wallet SET Solde = Solde + ? WHERE idUser = ?;";
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_double(stmt, 1, amount);
    sqlite3_bind_int(stmt, 2, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    return true;
}

bool UserRepository::processOrder(int userId, int drinkId, int quantity, std::string& errorMsg, double& newBalance) {
    if (quantity <= 0) {
        errorMsg = "Quantite invalide";
        return false;
    }

    auto user = getUserById(userId);
    auto drink = getDrinkById(drinkId);
    if (!user || !drink) {
        errorMsg = "Utilisateur ou boisson introuvable";
        return false;
    }

    if (drink->stock < quantity) {
        errorMsg = "Stock insuffisant pour " + drink->name;
        return false;
    }

    double totalPrice = drink->price * quantity;
    if (user->balance < totalPrice) {
        errorMsg = "Solde insuffisant";
        return false;
    }

    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        errorMsg = "Erreur base de donnees";
        return false;
    }

    // 1. Assurer Wallet et debiter
    const char* ensureSql = "INSERT OR IGNORE INTO Wallet (idUser, Solde) VALUES (?, 0.0);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, ensureSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "Erreur interne";
        return false;
    }
    sqlite3_bind_int(stmt, 1, userId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "Erreur interne";
        return false;
    }

    const char* debitSql = "UPDATE Wallet SET Solde = Solde - ? WHERE idUser = ?;";
    if (sqlite3_prepare_v2(db, debitSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "Erreur interne";
        return false;
    }
    sqlite3_bind_double(stmt, 1, totalPrice);
    sqlite3_bind_int(stmt, 2, userId);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "Echec du debit";
        return false;
    }

    // 2. Decrementer stock
    const char* stockSql = "UPDATE Drinks SET QuantityStock = QuantityStock - ? WHERE idDrink = ? AND QuantityStock >= ?;";
    if (sqlite3_prepare_v2(db, stockSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "Erreur interne";
        return false;
    }
    sqlite3_bind_int(stmt, 1, quantity);
    sqlite3_bind_int(stmt, 2, drinkId);
    sqlite3_bind_int(stmt, 3, quantity);
    rc = sqlite3_step(stmt);
    int stockChanges = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE || stockChanges == 0) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "Stock insuffisant";
        return false;
    }

    // 3. Enregistrer dans Historic
    std::string desc = "Achat de " + std::to_string(quantity) + " " + drink->name;
    const char* histSql = "INSERT INTO Historic (idUser, idDrink, Quantity, datetime, Description) VALUES (?, ?, ?, datetime('now','localtime'), ?);";
    if (sqlite3_prepare_v2(db, histSql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "Erreur interne";
        return false;
    }
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, drinkId);
    sqlite3_bind_int(stmt, 3, quantity);
    sqlite3_bind_text(stmt, 4, desc.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        errorMsg = "Echec enregistrement historique";
        return false;
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    auto updated = getUserById(userId);
    newBalance = updated ? updated->balance : (user->balance - totalPrice);
    return true;
}

std::vector<Drink> UserRepository::getDrinks() {
    std::vector<Drink> drinks;
    const char* sql = "SELECT idDrink, Name, Category, QuantityStock, Price FROM Drinks ORDER BY idDrink ASC;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            drinks.push_back(Drink{
                sqlite3_column_int(stmt, 0),
                colText(stmt, 1),
                colText(stmt, 2),
                sqlite3_column_int(stmt, 3),
                sqlite3_column_double(stmt, 4)
            });
        }
    }
    sqlite3_finalize(stmt);
    return drinks;
}

std::optional<Drink> UserRepository::getDrinkById(int id) {
    const char* sql = "SELECT idDrink, Name, Category, QuantityStock, Price FROM Drinks WHERE idDrink = ?;";
    sqlite3_stmt* stmt = nullptr;
    std::optional<Drink> drink;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            drink = Drink{
                sqlite3_column_int(stmt, 0),
                colText(stmt, 1),
                colText(stmt, 2),
                sqlite3_column_int(stmt, 3),
                sqlite3_column_double(stmt, 4)
            };
        }
    }
    sqlite3_finalize(stmt);
    return drink;
}

std::vector<HistoryEntry> UserRepository::getUserHistory(int userId) {
    std::vector<HistoryEntry> history;
    const char* sql =
        "SELECT H.idHistoric, H.idUser, H.idDrink, COALESCE(D.Name, 'Boisson'), "
        "H.Quantity, COALESCE(H.datetime, ''), COALESCE(H.Description, '') "
        "FROM Historic H "
        "LEFT JOIN Drinks D ON H.idDrink = D.idDrink "
        "WHERE H.idUser = ? "
        "ORDER BY H.datetime DESC, H.idHistoric DESC "
        "LIMIT 50;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, userId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            history.push_back(HistoryEntry{
                sqlite3_column_int(stmt, 0),
                sqlite3_column_int(stmt, 1),
                sqlite3_column_int(stmt, 2),
                colText(stmt, 3),
                sqlite3_column_int(stmt, 4),
                colText(stmt, 5),
                colText(stmt, 6)
            });
        }
    }
    sqlite3_finalize(stmt);
    return history;
}

std::string UserRepository::exportHistoryToXML(std::optional<int> userId) {
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<historics>\n";

    std::string sql =
        "SELECT H.idHistoric, H.idUser, COALESCE(U.Name, ''), H.idDrink, "
        "COALESCE(D.Name, ''), H.Quantity, COALESCE(H.datetime, ''), COALESCE(H.Description, '') "
        "FROM Historic H "
        "LEFT JOIN Users U ON H.idUser = U.idUser "
        "LEFT JOIN Drinks D ON H.idDrink = D.idDrink ";
    if (userId.has_value()) {
        sql += "WHERE H.idUser = ? ";
    }
    sql += "ORDER BY H.datetime DESC, H.idHistoric DESC;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (userId.has_value()) {
            sqlite3_bind_int(stmt, 1, *userId);
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            xml << "  <order>\n"
                << "    <id>" << sqlite3_column_int(stmt, 0) << "</id>\n"
                << "    <idUser>" << sqlite3_column_int(stmt, 1) << "</idUser>\n"
                << "    <userName>" << xmlEscape(colText(stmt, 2)) << "</userName>\n"
                << "    <idDrink>" << sqlite3_column_int(stmt, 3) << "</idDrink>\n"
                << "    <drinkName>" << xmlEscape(colText(stmt, 4)) << "</drinkName>\n"
                << "    <quantity>" << sqlite3_column_int(stmt, 5) << "</quantity>\n"
                << "    <datetime>" << xmlEscape(colText(stmt, 6)) << "</datetime>\n"
                << "    <description>" << xmlEscape(colText(stmt, 7)) << "</description>\n"
                << "  </order>\n";
        }
    }
    sqlite3_finalize(stmt);
    xml << "</historics>\n";
    return xml.str();
}
