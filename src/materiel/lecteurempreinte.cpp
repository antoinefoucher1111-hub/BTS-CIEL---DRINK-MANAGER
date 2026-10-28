#include "lecteurempreinte.h"
#include "protocole.h"

bool LecteurEmpreinte::ouvrir(const std::string&, int)
{
    // TODO: ouvrir le port série (Windows: CreateFile + SetCommState ; Linux: termios)
    return false;
}

void LecteurEmpreinte::fermer()
{
    // TODO: fermer le port
}

void LecteurEmpreinte::lire()
{
    // TODO: lire les octets disponibles sur le port (non bloquant),
    // puis appeler recevoir(octets).
}

bool LecteurEmpreinte::envoyer(const std::string& commande)
{
    if (commande.empty()) return false;
    // TODO: écrire `commande` sur le port ; renvoyer false si le port est fermé.
    return false;
}

bool LecteurEmpreinte::afficherCode(const std::string& code)
{
    return envoyer(Protocole::commandeCode(code));
}

bool LecteurEmpreinte::effacerAffichage()
{
    return envoyer(Protocole::commandeClear());
}

bool LecteurEmpreinte::demanderEnrolement(int idUser)
{
    return envoyer(Protocole::commandeEnroll(idUser));
}

void LecteurEmpreinte::recevoir(const std::string& octets)
{
    m_tampon += octets;
    std::size_t fin;
    while ((fin = m_tampon.find('\n')) != std::string::npos) {
        traiterLigne(m_tampon.substr(0, fin));
        m_tampon.erase(0, fin + 1);
    }
    if (m_tampon.size() > 256)      // pas de '\n' : on jette pour ne pas grossir sans fin
        m_tampon.clear();
}

void LecteurEmpreinte::simulerLigne(const std::string& ligne)
{
    traiterLigne(ligne);
}

void LecteurEmpreinte::traiterLigne(const std::string& ligne)
{
    const Protocole::Message m = Protocole::analyser(ligne);
    switch (m.type) {
    case Protocole::Type::UserOk:
        if (surUtilisateurIdentifie) surUtilisateurIdentifie(m.id);
        break;
    case Protocole::Type::AuthFail:
        if (surAuthEchouee) surAuthEchouee();
        break;
    case Protocole::Type::EnrollOk:
        if (surEnrolement) surEnrolement(true, m.id);
        break;
    case Protocole::Type::EnrollFail:
        if (surEnrolement) surEnrolement(false, 0);
        break;
    case Protocole::Type::Invalide:
        if (surErreur) surErreur("Ligne invalide : " + ligne.substr(0, 40));
        break;
    }
}