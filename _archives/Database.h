#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>

struct UserProfile {
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

class DatabaseManager {
private:
    sqlite3* db;

public:
    DatabaseManager(const std::string& db_name);
    ~DatabaseManager();

    void initializeTables();
    void seedData(); 

    UserProfile getUserProfile(int userId);
    std::vector<Drink> getAllDrinks();
    bool updateUserName(int userId, const std::string& newName);
    bool updateUserBalance(int userId, double amountToAdd);
    bool processOrder(int userId, int drinkId, double price);
    
    std::string exportHistoricToXML();
};

#endif