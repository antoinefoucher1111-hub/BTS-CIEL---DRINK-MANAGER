#include "portefeuille.h"

bool Portefeuille::peutDebiter(int soldeCentimes, int prixCentimes)
{
    return prixCentimes > 0 && prixCentimes <= soldeCentimes;
}

bool Portefeuille::peutRecharger(int soldeCentimes, int montantCentimes)
{
   if (montantCentimes > 0 && montantCentimes <= RECHARGE_MAX && (soldeCentimes + montantCentimes) <= SOLDE_MAX) {
       return true;
   }
   return false;
}

int Portefeuille::apresDebit(int soldeCentimes, int prixCentimes)
{
    if (peutDebiter(soldeCentimes, prixCentimes)) {
        return soldeCentimes - prixCentimes;
    }
    return soldeCentimes;
}

int Portefeuille::apresRecharge(int soldeCentimes, int montantCentimes)
{
    if (peutRecharger(soldeCentimes, montantCentimes)) {
        return soldeCentimes + montantCentimes;
    }
    return soldeCentimes;
}
