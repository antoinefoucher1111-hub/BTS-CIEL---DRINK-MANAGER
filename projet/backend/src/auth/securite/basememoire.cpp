#include "basememoire.h"

BaseMemoire::BaseMemoire(const std::map<int, int>& empreintes)
    : m_empreintes(empreintes)
{
}

std::optional<int> BaseMemoire::trouverUtilisateurParEmpreinte(int fingerprintId)
{
    auto it = m_empreintes.find(fingerprintId);
    if (it == m_empreintes.end())
    {
        return std::nullopt;
    }
    return it->second;
}

void BaseMemoire::enregistrerJeton(const Jeton& j)
{
    m_jetons[j.idUser] = j;
}

std::optional<Jeton> BaseMemoire::chargerJeton(int idUser)
{
    auto it = m_jetons.find(idUser);
    if (it == m_jetons.end())
    {
        return std::nullopt;
    }
    return it->second;
}

void BaseMemoire::mettreAJourJeton(const Jeton& j)
{
    m_jetons[j.idUser] = j;
}