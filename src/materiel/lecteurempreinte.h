#pragma once
#include <cstdint>
#include <functional>
#include <string>

class LecteurEmpreinte {
public:
    ~LecteurEmpreinte();

    LecteurEmpreinte() = default;
    LecteurEmpreinte(const LecteurEmpreinte&) = delete;
    LecteurEmpreinte& operator=(const LecteurEmpreinte&) = delete;

    // Callbacks à renseigner par l'Authentificateur
    std::function<void(int)>                surUtilisateurIdentifie;  // idUser
    std::function<void()>                   surAuthEchouee;
    std::function<void(bool, int)>          surEnrolement;            // (réussi, idUser)
    std::function<void(const std::string&)> surErreur;

    bool ouvrir(const std::string& nomPort, int baud = 9600);
    void fermer();

    // PC -> Arduino (false si argument invalide ou port fermé)
    bool afficherCode(const std::string& code);
    bool effacerAffichage();
    bool demanderEnrolement(int idUser);

    // Arduino -> PC
    void lire();                                 // lit les octets disponibles, puis appelle recevoir()
    void recevoir(const std::string& octets);    // accumule, traite chaque ligne complète
    void simulerLigne(const std::string& ligne); // test sans Arduino

private:
    void traiterLigne(const std::string& ligne);
    bool envoyer(const std::string& commande);

    std::intptr_t m_port = -1;
    std::string m_tampon;
    bool m_rejeterLigneTropLongue = false;
};