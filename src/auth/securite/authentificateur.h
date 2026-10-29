#pragma once
#include <string>
#include <ctime>
#include "ibase.h"
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
    Authentificateur(IBase& base, IAfficheur& afficheur);

    bool surUtilisateurIdentifie(int fingerprintId, std::time_t maintenant);

    ResultatVerif verifier(int idUser, const std::string& codeSaisi, std::time_t maintenant);

private:
    IBase&      m_base;
    IAfficheur& m_afficheur;
};