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
};

struct Drink {
    int id;
    std::string name;
    std::string category;
    int stock;
    double price;
};

// Déclaration de la classe UserRepository
class UserRepository {
private:
    sqlite3* db;

public:
    // Constructeur
    UserRepository(sqlite3* db);

    // Méthodes requises par ton main.cpp
    std::optional<User> getUserById(int id);
    bool updateUserName(int id, const std::string& name);
    bool updateBalance(int id, double amount);
    bool recordPurchase(int userId, int drinkId, int quantity, const std::string& description);
    std::vector<Drink> getDrinks();
    std::optional<Drink> getDrinkById(int id);
};