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
    std::string code = Securite::genererCode();
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

