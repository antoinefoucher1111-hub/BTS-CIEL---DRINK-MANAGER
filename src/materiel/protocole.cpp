#include "protocole.h"
#include <cctype>

namespace {
std::string nettoyer(const std::string& s) {       // enlève \r, \n, espaces aux bords
    std::size_t debut = 0, fin = s.size();
    while (debut < fin && std::isspace(static_cast<unsigned char>(s[debut]))) ++debut;
    while (fin > debut && std::isspace(static_cast<unsigned char>(s[fin - 1]))) --fin;
    return s.substr(debut, fin - debut);
}

bool lireId(const std::string& texte, int& id) {   // chiffres uniquement, 1..ID_MAX
    if (texte.empty() || texte.size() > 3) return false;
    for (char c : texte)
        if (c < '0' || c > '9') return false;
    id = std::stoi(texte);
    return id >= 1 && id <= Protocole::ID_MAX;
}
}

Protocole::Message Protocole::analyser(const std::string& brut)
{
    if (brut.size() > LONGUEUR_MAX + 2) return {};  // +2 pour le \r\n ; évite de traiter 5000 caractères
    const std::string ligne = nettoyer(brut);
    if (ligne.empty() || ligne.size() > LONGUEUR_MAX) return {};

    if (ligne == "AUTH_FAIL")   return { Type::AuthFail, 0 };
    if (ligne == "ENROLL_FAIL") return { Type::EnrollFail, 0 };

    int id = 0;
    if (ligne.rfind("USER:", 0) == 0) {             // rfind(...,0)==0 : commence par
        if (lireId(ligne.substr(5), id)) return { Type::UserOk, id };
        return {};
    }
    if (ligne.rfind("ENROLL_OK:", 0) == 0) {
        if (lireId(ligne.substr(10), id)) return { Type::EnrollOk, id };
        return {};
    }
    return {};
}

bool Protocole::codeValide(const std::string& code)
{
    if (code.size() != LONGUEUR_CODE) return false;
    for (char c : code)
        if (c < '0' || c > '9') return false;
    return true;
}

std::string Protocole::commandeCode(const std::string& code)
{
    return codeValide(code) ? "CODE:" + code + "\n" : "";
}

std::string Protocole::commandeClear() { return "CLEAR\n"; }

std::string Protocole::commandeEnroll(int idUser)
{
    if (idUser < 1 || idUser > ID_MAX) return "";
    return "ENROLL:" + std::to_string(idUser) + "\n";
}