#pragma once
#include <map>
#include <set>
#include <string>
#include <ctime>
#include "jeton.h"

class IAfficheur
{
public:
    virtual ~IAfficheur() = default;
    virtual void afficher(const std::string& texte) = 0;
};

class Authentificateur
{
public:
    Authentificateur(IAfficheur& afficheur, const std::set<int>& empreintesConnues);

    bool surUtilisateurIdentifie(int fingerprintId, std::time_t maintenant);
    ResultatVerif verifier(int fingerprintId, const std::string& codeSaisi, std::time_t maintenant);

private:
    IAfficheur&          m_afficheur;
    std::set<int>        m_connus;
    std::map<int, Jeton> m_jetons;
};