#include "portefeuille.h"
#include <iostream>
#include <string>

void test_assert(const std::string& portefeuille, bool condition) {
    const std::string ROUGE = "\033[31m";
    const std::string VERT = "\033[32m";
    const std::string RESET = "\033[0m";

    if (condition) {
        std::cout << VERT << "[SUCCÈS] " << RESET << portefeuille << std::endl;
    } else {
        std::cout << ROUGE << "[ÉCHEC]  " << RESET << portefeuille << std::endl;
    }
}

int main() {
    std::cout << "--- TESTS : Portefeuille::peutDebiter ---\n";
    // Cas normaux
    test_assert("Achat possible si solde suffisant", Portefeuille::peutDebiter(1000, 500) == true);
    test_assert("Achat possible si solde exact", Portefeuille::peutDebiter(500, 500) == true);
    // Cas d'erreurs
    test_assert("Achat refusé si solde insuffisant", Portefeuille::peutDebiter(500, 1000) == false);
    test_assert("Achat refusé si prix négatif ou nul", Portefeuille::peutDebiter(1000, -100) == false);

    std::cout << "\n--- TESTS : Portefeuille::peutRecharger ---\n";
    // Cas normaux
    test_assert("Recharge de 50 EUR possible", Portefeuille::peutRecharger(1000, 5000) == true);
    // Cas limites (Plafonds)
    test_assert("Recharge refusée si > RECHARGE_MAX", Portefeuille::peutRecharger(1000, 15000) == false);
    test_assert("Recharge refusée si nouveau solde > SOLDE_MAX", Portefeuille::peutRecharger(45000, 6000) == false);
    // Cas d'erreurs
    test_assert("Recharge refusée si montant négatif", Portefeuille::peutRecharger(1000, -500) == false);

    std::cout << "\n--- TESTS : Portefeuille::apresDebit ---\n";
    test_assert("Débit de 5 EUR sur 10 EUR -> reste 5 EUR", Portefeuille::apresDebit(1000, 500) == 500);
    test_assert("Débit refusé (trop cher) -> solde inchangé", Portefeuille::apresDebit(1000, 5000) == 1000);

    std::cout << "\n--- TESTS : Portefeuille::apresRecharge ---\n";
    test_assert("Recharge 10 EUR sur 10 EUR -> donne 20 EUR", Portefeuille::apresRecharge(1000, 1000) == 2000);
    test_assert("Recharge refusée (plafond atteint) -> solde inchangé", Portefeuille::apresRecharge(45000, 10000) == 45000);

    return 0;
}