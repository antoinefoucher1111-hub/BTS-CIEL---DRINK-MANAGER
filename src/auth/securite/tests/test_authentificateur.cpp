#include <iostream>
#include "authentificateur.h"
#include "basememoire.h"

class AfficheurConsole : public IAfficheur
{
public:
    std::string dernierCode;
    void afficher(const std::string& texte) override
    {
        dernierCode = texte;
        std::cout << "[AFFICHEUR] " << texte << std::endl;
    }
};

int main()
{
    BaseMemoire base({{42, 1}, {7, 2}});   // empreinte 42 -> user 1, empreinte 7 -> user 2
    AfficheurConsole aff;
    Authentificateur auth(base, aff);
    std::time_t t = 1000;

    std::cout << "1 inconnu 99 (att 0)       : " << auth.surUtilisateurIdentifie(99, t) << std::endl;
    std::cout << "2 connu 42 (att 1)         : " << auth.surUtilisateurIdentifie(42, t) << std::endl;
    std::string code = aff.dernierCode;

    std::cout << "3 faux code (att 1)        : " << static_cast<int>(auth.verifier(1, "abcdef", t + 5)) << std::endl;
    std::cout << "4 tentatives en base (att 1): " << base.chargerJeton(1)->tentatives << std::endl;
    std::cout << "5 bon code (att 0)         : " << static_cast<int>(auth.verifier(1, code, t + 10)) << std::endl;
    std::cout << "6 rejouer (att 2)          : " << static_cast<int>(auth.verifier(1, code, t + 11)) << std::endl;

    // brute-force : 3 echecs puis le bon code
    auth.surUtilisateurIdentifie(7, t);
    std::string code7 = aff.dernierCode;
    auth.verifier(2, "000001", t + 1);
    auth.verifier(2, "000002", t + 2);
    auth.verifier(2, "000003", t + 3);
    std::cout << "7 bon apres 3 echecs (att 2): " << static_cast<int>(auth.verifier(2, code7, t + 4)) << std::endl;

    // un code ne marche que pour son utilisateur
    auth.surUtilisateurIdentifie(42, t + 20);
    std::string code42 = aff.dernierCode;
    std::cout << "8 code d'un autre (att 1 ou 2): " << static_cast<int>(auth.verifier(2, code42, t + 21)) << std::endl;
    return 0;
}