#pragma once
#include <sqlite3.h>
#include <optional>
#include <vector>
#include <string>

struct User {
    int id = 0;
    std::string name;
    int accessPrint = 0;
    double balance = 0.0;
};

struct Drink {
    int id = 0;
    std::string name;
    std::string category;
    int stock = 0;
    double price = 0.0;
};

class UserRepository {
private:
    sqlite3* db;

public:
    explicit UserRepository(sqlite3* database);

    std::optional<User> getUserById(int userId);
    bool updateUserName(int userId, const std::string& newName);
    bool updateBalance(int userId, double delta);
    bool recordPurchase(int userId, int drinkId, int quantity, const std::string& description);
    
    std::vector<Drink> getDrinks();
    std::optional<Drink> getDrinkById(int drinkId);
};