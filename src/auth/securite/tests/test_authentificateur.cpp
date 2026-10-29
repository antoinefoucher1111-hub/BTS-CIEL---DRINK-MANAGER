#include <iostream>
#include "authentificateur.h"

class AfficheurMemoire : public IAfficheur
{
public:
    std::string dernier;
    int nbAppels = 0;

    void afficher(const std::string& texte) override
    {
        dernier = texte;
        nbAppels++;
    }
};

int main()
{
    std::time_t t = 1000;
    AfficheurMemoire afficheur;
    std::set<int> connus = {42, 7};
    Authentificateur auth(afficheur, connus, Securite{}, GestionJeton{});

    // ----- Partie 1 : surUtilisateurIdentifie -----
    std::cout << "--- surUtilisateurIdentifie ---" << std::endl;

    bool ok42 = auth.surUtilisateurIdentifie(42, t);
    std::string code42 = afficheur.dernier;
    std::cout << "connu 42 (attendu 1)      : " << ok42 << std::endl;
    std::cout << "affichages (attendu 1)    : " << afficheur.nbAppels << std::endl;
    std::cout << "longueur code (attendu 6) : " << code42.size() << std::endl;

    bool inconnu = auth.surUtilisateurIdentifie(99, t);
    std::cout << "inconnu 99 (attendu 0)    : " << inconnu << std::endl;
    std::cout << "affichages (attendu 1)    : " << afficheur.nbAppels << std::endl;

    bool ok7 = auth.surUtilisateurIdentifie(7, t);
    std::string code7 = afficheur.dernier;
    std::cout << "connu 7 (attendu 1)       : " << ok7 << std::endl;
    std::cout << "affichages (attendu 2)    : " << afficheur.nbAppels << std::endl;

    // ----- Partie 2 : verifier (0=OK, 1=CODE_FAUX, 2=EXPIRE_OU_BLOQUE) -----
    // Le jeton reste dans la map jusqu'a expiration : un bon code peut reservir.
    std::cout << "--- verifier ---" << std::endl;

    std::cout << "1 code absent (attendu 1)           : "
              << static_cast<int>(auth.verifier("abcdef", t)) << std::endl;
    std::cout << "2 bon code 42 (attendu 0)           : "
              << static_cast<int>(auth.verifier(code42, t)) << std::endl;
    std::cout << "3 meme code (attendu 0)             : "
              << static_cast<int>(auth.verifier(code42, t)) << std::endl;
    std::cout << "4 bon code 7 (attendu 0)            : "
              << static_cast<int>(auth.verifier(code7, t)) << std::endl;
    std::cout << "5 code 42 a t+59 (attendu 0)        : "
              << static_cast<int>(auth.verifier(code42, t + 59)) << std::endl;
    std::cout << "6 code 42 a t+60 (attendu 2)        : "
              << static_cast<int>(auth.verifier(code42, t + 60)) << std::endl;
    std::cout << "7 code 42 apres expiration (att. 1) : "
              << static_cast<int>(auth.verifier(code42, t)) << std::endl;
    std::cout << "8 code 7 encore valide (attendu 0)  : "
              << static_cast<int>(auth.verifier(code7, t + 59)) << std::endl;

    // Deuxieme passage du meme doigt : l'ancien code reste valable.
    AfficheurMemoire afficheur2;
    Authentificateur auth2(afficheur2, connus, Securite{}, GestionJeton{});
    auth2.surUtilisateurIdentifie(42, t);
    std::string premier = afficheur2.dernier;
    auth2.surUtilisateurIdentifie(42, t);
    std::string second = afficheur2.dernier;
    std::cout << "9 ancien code encore OK (attendu 0) : "
              << static_cast<int>(auth2.verifier(premier, t)) << std::endl;
    std::cout << "10 nouveau code OK (attendu 0)      : "
              << static_cast<int>(auth2.verifier(second, t)) << std::endl;

    AfficheurMemoire afficheur3;
    std::set<int> vide;
    Authentificateur auth3(afficheur3, vide, Securite{}, GestionJeton{});
    std::cout << "11 aucun connu (attendu 0)          : "
              << auth3.surUtilisateurIdentifie(42, t) << std::endl;
    std::cout << "affichages (attendu 0)              : " << afficheur3.nbAppels << std::endl;

    return 0;
}
