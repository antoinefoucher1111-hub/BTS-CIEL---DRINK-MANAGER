#pragma once
#include <string>
#include <ctime>

struct Jeton
{
    int         idUser = 0;
    std::string codeHash;
    std::time_t expiresAt = 0;
    bool        used = false;
    int         tentatives = 0;
};

enum class ResultatVerif { OK, CODE_FAUX, EXPIRE_OU_BLOQUE };

class GestionJeton
{
public:
    static constexpr int DUREE_SECONDES = 60;
    static constexpr int MAX_TENTATIVES = 3;

    static Jeton creer(int idUser, const std::string& code, std::time_t maintenant);
    static bool  estValide(const Jeton& j, std::time_t maintenant);
    static ResultatVerif verifierCode(Jeton& j, const std::string& codeSaisi, std::time_t maintenant);
};
