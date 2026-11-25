#pragma once
#include <optional>
#include "jeton.h"

class IBase
{
public:
    virtual ~IBase() = default;

    // idUser si ce numero d'empreinte existe, sinon rien
    virtual std::optional<int>   trouverUtilisateurParEmpreinte(int fingerprintId) = 0;
    // enregistre le jeton (remplace l'ancien du meme utilisateur)
    virtual void                 enregistrerJeton(const Jeton& j) = 0;
    virtual std::optional<Jeton> chargerJeton(int idUser) = 0;
    // sauvegarde used et tentatives
    virtual void                 mettreAJourJeton(const Jeton& j) = 0;
};