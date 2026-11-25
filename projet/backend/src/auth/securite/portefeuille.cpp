#include "portefeuille.h"

bool Portefeuille::peutDebiter(int soldeCentimes, int prixCentimes)
{
    return soldeCentimes >= 0 && prixCentimes > 0 && prixCentimes <= soldeCentimes;
}

bool Portefeuille::peutRecharger(int soldeCentimes, int montantCentimes)
{
    return soldeCentimes >= 0
        && montantCentimes > 0
        && montantCentimes <= RECHARGE_MAX
        && soldeCentimes <= (SOLDE_MAX - montantCentimes);
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
