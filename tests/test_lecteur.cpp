#include <iostream>
#include "../src/materiel/lecteurempreinte.h"

static int echecs = 0;
#define VERIFIER(cond) do { if (!(cond)) { ++echecs; \
    std::cerr << "ECHEC ligne " << __LINE__ << " : " #cond "\n"; } } while (0)

int main() {
    LecteurEmpreinte l;
    int id = 0; bool refus = false, enrolOk = false; int erreurs = 0;
    l.surUtilisateurIdentifie = [&](int i) { id = i; };
    l.surAuthEchouee          = [&]() { refus = true; };
    l.surEnrolement           = [&](bool ok, int) { enrolOk = ok; };
    l.surErreur               = [&](const std::string&) { ++erreurs; };

    l.recevoir("USER:4");   VERIFIER(id == 0);    // ligne incomplète : rien ne se passe
    l.recevoir("2\r\n");    VERIFIER(id == 42);   // la suite arrive
    l.recevoir("AUTH_FAIL\nENROLL_OK:7\n");       // deux lignes d'un coup
    VERIFIER(refus); VERIFIER(enrolOk);
    l.simulerLigne("USER:abc"); VERIFIER(erreurs == 1);

    VERIFIER(!l.afficherCode("12ab56"));          // invalide : rien d'envoyé
    VERIFIER(!l.demanderEnrolement(0));

    std::cout << (echecs == 0 ? "Tous les tests passent\n" : "Des tests echouent\n");
    return echecs == 0 ? 0 : 1;
}