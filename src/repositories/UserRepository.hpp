// src/repositories/UserRepository.hpp
#pragma once
#include <sqlite3.h>
#include <optional>
#include <vector>
#include <string>

struct User {
    int id = 0;
    int fingerprintId = -1;
    std::string name;
    std::string email;
    double balance = 0.0;
    bool isAdmin = false;
};

struct Drink {
    int id = 0;
    std::string name;
    double price = 0.0;
};

class UserRepository {
private:
    sqlite3* db;

public:
    explicit UserRepository(sqlite3* database);
    void initTables();
    
    std::optional<User> getUserById(int userId);
    bool updateUserProfile(int userId, const std::string& name);
    bool updateBalance(int userId, double delta);
    std::vector<Drink> getDrinks();
    std::optional<Drink> getDrinkById(int drinkId);
};
