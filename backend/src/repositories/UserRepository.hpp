#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../../sqlite/sqlite3.h"

// Structures de données (l'ORM)
struct User {
    int id;
    std::string name;
    double balance;
    int accessPrint = 0;
};

struct Drink {
    int id;
    std::string name;
    std::string category;
    int stock;
    double price;
};

struct HistoryEntry {
    int id;
    int userId;
    int drinkId;
    std::string drinkName;
    int quantity;
    std::string datetime;
    std::string description;
};

// Déclaration de la classe UserRepository
class UserRepository {
private:
    sqlite3* db;

public:
    UserRepository(sqlite3* db);

    std::vector<User> getAllUsers();
    std::optional<User> getUserById(int id);
    std::optional<User> getUserByAccessPrint(int accessPrint);
    std::optional<User> createUser(const std::string& name, double initialBalance = 0.0, int accessPrint = 0);
    bool updateUserName(int id, const std::string& name);
    bool updateUserAccessPrint(int id, int accessPrint);
    bool updateBalance(int id, double amount);
    bool processOrder(int userId, int drinkId, int quantity, std::string& errorMsg, double& newBalance);
    std::vector<Drink> getDrinks();
    std::optional<Drink> getDrinkById(int id);
    std::vector<HistoryEntry> getUserHistory(int userId);
    std::string exportHistoryToXML(std::optional<int> userId = std::nullopt);
};