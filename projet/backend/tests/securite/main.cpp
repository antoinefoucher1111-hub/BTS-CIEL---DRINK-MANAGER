#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include "authentificateur.h"
#include "basememoire.h"
#include "jeton.h"
#include "portefeuille.h"
#include "securite.h"

class AfficheurTest : public IAfficheur
{
public:
    std::string dernierCode;
    void afficher(const std::string& texte) override
    {
        dernierCode = texte;
    }
};

int main()
{
    // 1. Securite : hash SHA-256 et generation de code 6 chiffres
    assert(Securite::hasher("123456") ==
           "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92");

    for (int i = 0; i < 10; ++i)
    {
        std::string code = Securite::genererCode();
        assert(code.size() == 6);
        for (char c : code)
        {
            assert(c >= '0' && c <= '9');
        }
    }

    // 2. Authentificateur + BaseMemoire + GestionJeton (flux par idUser et flux web par code)
    BaseMemoire base({{1, 10}, {2, 20}});
    AfficheurTest afficheur;
    Authentificateur auth(base, afficheur);

    assert(!auth.surUtilisateurIdentifie(99, 1000));
    assert(auth.surUtilisateurIdentifie(1, 1000));
    const std::string codeUser1 = afficheur.dernierCode;
    assert(codeUser1.size() == 6);

    // Verification par idUser : faux code -> CODE_FAUX, bon code -> OK, reutilisation -> EXPIRE_OU_BLOQUE
    const std::string wrong1 = (codeUser1 == "000000") ? "111111" : "000000";
    assert(auth.verifier(10, wrong1, 1010) == ResultatVerif::CODE_FAUX);
    assert(auth.verifier(10, codeUser1, 1020) == ResultatVerif::OK);
    assert(auth.verifier(10, codeUser1, 1021) == ResultatVerif::EXPIRE_OU_BLOQUE);

    // Verification par code seul (utilisee par /api/auth/verify-code)
    assert(auth.surUtilisateurIdentifie(2, 2000));
    const std::string codeUser2 = afficheur.dernierCode;
    assert(!auth.verifierParCode((codeUser2 == "000000") ? "111111" : "000000", 2010).has_value());
    auto matched = auth.verifierParCode(codeUser2, 2050);
    assert(matched.has_value() && *matched == 20);
    // Usage unique
    assert(!auth.verifierParCode(codeUser2, 2051).has_value());

    // Expiration apres DUREE_SECONDES (120s)
    assert(auth.surUtilisateurIdentifie(1, 3000));
    assert(!auth.verifierParCode(afficheur.dernierCode, 3000 + GestionJeton::DUREE_SECONDES).has_value());

    // Blocage apres MAX_TENTATIVES (3) essais faux
    assert(auth.surUtilisateurIdentifie(1, 4000));
    const std::string wrong2 = (afficheur.dernierCode == "000000") ? "111111" : "000000";
    assert(auth.verifier(10, wrong2, 4001) == ResultatVerif::CODE_FAUX);
    assert(auth.verifier(10, wrong2, 4002) == ResultatVerif::CODE_FAUX);
    assert(auth.verifier(10, wrong2, 4003) == ResultatVerif::CODE_FAUX);
    assert(auth.verifier(10, afficheur.dernierCode, 4004) == ResultatVerif::EXPIRE_OU_BLOQUE);

    // 3. Portefeuille (debit, recharge, plafonds et protection debordement)
    assert(Portefeuille::peutDebiter(1000, 180));
    assert(!Portefeuille::peutDebiter(100, 180));
    assert(!Portefeuille::peutDebiter(1000, 0));
    assert(!Portefeuille::peutDebiter(-50, 10));
    assert(Portefeuille::apresDebit(1000, 180) == 820);
    assert(Portefeuille::apresDebit(100, 180) == 100);

    assert(Portefeuille::peutRecharger(1000, 10000));
    assert(!Portefeuille::peutRecharger(1000, 10001));
    assert(!Portefeuille::peutRecharger(45000, 6000));
    assert(!Portefeuille::peutRecharger(std::numeric_limits<int>::max(), 100));
    assert(Portefeuille::apresRecharge(45000, 5000) == 50000);
    assert(Portefeuille::apresRecharge(45000, 6000) == 45000);

    std::cout << "[OK] Tous les tests du module securite ont reussi." << std::endl;
    return 0;
}
