#include "jeton.h"
#include "securite.h"

Jeton GestionJeton::creer(int idUser, const std::string& code, std::time_t maintenant)
{
    Jeton j;
    j.idUser = idUser;
    j.codeHash = Securite::hasher(code);
    j.expiresAt = maintenant + DUREE_SECONDES;
    j.used = false;
    j.tentatives = 0;
    return j;
}

bool GestionJeton::estValide(const Jeton& j, std::time_t maintenant)
{
    return !j.used && j.tentatives < MAX_TENTATIVES && maintenant < j.expiresAt;
}

ResultatVerif GestionJeton::verifierCode(Jeton& j, const std::string& codeSaisi, std::time_t maintenant)
{
    if (!estValide(j, maintenant))
    {
        return ResultatVerif::EXPIRE_OU_BLOQUE;
    }

    if (Securite::hasher(codeSaisi) == j.codeHash)
    {
        j.used = true;
        return ResultatVerif::OK;
    }

    j.tentatives++;
    return ResultatVerif::CODE_FAUX;
}