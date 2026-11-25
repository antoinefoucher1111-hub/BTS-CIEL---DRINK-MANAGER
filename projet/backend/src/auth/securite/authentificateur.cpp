#include "authentificateur.h"
#include "securite.h"

Authentificateur::Authentificateur(IBase& base, IAfficheur& afficheur)
    : m_base(base), m_afficheur(afficheur)
{
}

bool Authentificateur::surUtilisateurIdentifie(int fingerprintId, std::time_t maintenant)
{
    auto idUser = m_base.trouverUtilisateurParEmpreinte(fingerprintId);
    if (!idUser)
    {
        return false;
    }

    // Garantit l'unicite du code parmi les jetons encore actifs
    std::string code;
    for (int essai = 0; essai < 10; ++essai)
    {
        code = Securite::genererCode();
        auto existant = m_base.chargerJetonParHash(Securite::hasher(code));
        if (!existant || !GestionJeton::estValide(*existant, maintenant) || existant->idUser == *idUser)
        {
            break;
        }
    }

    Jeton j = GestionJeton::creer(*idUser, code, maintenant);
    m_base.enregistrerJeton(j);
    m_afficheur.afficher(code);
    return true;
}

ResultatVerif Authentificateur::verifier(int idUser, const std::string& codeSaisi, std::time_t maintenant)
{
    auto jeton = m_base.chargerJeton(idUser);
    if (!jeton)
    {
        return ResultatVerif::EXPIRE_OU_BLOQUE;
    }
    ResultatVerif r = GestionJeton::verifierCode(*jeton, codeSaisi, maintenant);
    m_base.mettreAJourJeton(*jeton);
    return r;
}

std::optional<int> Authentificateur::verifierParCode(const std::string& codeSaisi, std::time_t maintenant)
{
    auto jeton = m_base.chargerJetonParHash(Securite::hasher(codeSaisi));
    if (!jeton)
    {
        return std::nullopt;
    }
    ResultatVerif r = GestionJeton::verifierCode(*jeton, codeSaisi, maintenant);
    m_base.mettreAJourJeton(*jeton);
    if (r != ResultatVerif::OK)
    {
        return std::nullopt;
    }
    return jeton->idUser;
}
