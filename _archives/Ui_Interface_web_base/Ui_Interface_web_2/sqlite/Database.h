#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>

// ---------------------------------------------------------
// ORM : Représentation Objet des tables du DER
// ---------------------------------------------------------

struct UserProfile {
    int id;           // Correspond à idUser (PK)
    std::string name; // Correspond à Name
    double balance;   // Correspond à Solde dans WALLET
};

struct Drink {
    int id;
    std::string name;
    std::string category;
    int stock;
    double price;
};

// ---------------------------------------------------------
// Gestionnaire de Base de Données (SQLite)
// ---------------------------------------------------------
class DatabaseManager {
private:
    sqlite3* db;

public:
    DatabaseManager(const std::string& db_name);
    ~DatabaseManager();

    // Initialisation
    void initializeTables();
    void seedData(); // Injecter des fausses données au premier lancement

    // ORM : Méthodes de lecture/écriture
    UserProfile getUserProfile(int userId);
    std::vector<Drink> getAllDrinks();
    bool updateUserName(int userId, const std::string& newName);
    bool updateUserBalance(int userId, double amountToAdd);
    bool processOrder(int userId, int drinkId, double price);
    
    // Fonction d'export XML
    std::string exportHistoricToXML();
};

#endif