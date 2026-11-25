#pragma once
#include <map>
#include "ibase.h"

class BaseMemoire : public IBase
{
public:
    explicit BaseMemoire(const std::map<int, int>& empreintes);

    std::optional<int>   trouverUtilisateurParEmpreinte(int fingerprintId) override;
    void                 enregistrerJeton(const Jeton& j) override;
    std::optional<Jeton> chargerJeton(int idUser) override;
    void                 mettreAJourJeton(const Jeton& j) override;

private:
    std::map<int, int>   m_empreintes;   // numero d'empreinte -> idUser
    std::map<int, Jeton> m_jetons;       // idUser -> jeton
};