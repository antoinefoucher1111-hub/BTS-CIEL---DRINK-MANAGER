#pragma once

class Portefeuille
{
public:
    static constexpr int SOLDE_MAX = 50000;      // 500,00 EUR
    static constexpr int RECHARGE_MAX = 10000;   // 100,00 EUR

    static bool peutDebiter(int soldeCentimes, int prixCentimes);
    static bool peutRecharger(int soldeCentimes, int montantCentimes);
    static int  apresDebit(int soldeCentimes, int prixCentimes);
    static int  apresRecharge(int soldeCentimes, int montantCentimes);
};