#pragma once
#include <cstddef>
#include <string>

class Protocole {
public:
    enum class Type { Invalide, UserOk, AuthFail, EnrollOk, EnrollFail };
    struct Message { Type type = Type::Invalide; int id = 0; };

    static constexpr int ID_MAX = 127;            // selon le capteur
    static constexpr std::size_t LONGUEUR_MAX = 32;
    static constexpr std::size_t LONGUEUR_CODE = 6;

    static Message analyser(const std::string& ligne);

    // PC -> Arduino : renvoient "" si l'argument est invalide
    static bool        codeValide(const std::string& code);   // exactement 6 chiffres
    static std::string commandeCode(const std::string& code); // "CODE:483920\n"
    static std::string commandeClear();                       // "CLEAR\n"
    static std::string commandeEnroll(int idUser);            // "ENROLL:42\n"
};
