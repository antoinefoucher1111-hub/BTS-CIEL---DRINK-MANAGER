#pragma once
#include <functional>
#include <string>

class LecteurEmpreinte {
public:
    // Callbacks à renseigner par l'Authentificateur
    std::function<void(int)>                surUtilisateurIdentifie;  // idUser
    std::function<void()>                   surAuthEchouee;
    std::function<void(bool, int)>          surEnrolement;            // (réussi, idUser)
    std::function<void(const std::string&)> surErreur;

    bool ouvrir(const std::string& nomPort, int baud = 9600);  // TODO: API système
    void fermer();                                             // TODO

    // PC -> Arduino (false si argument invalide ou port fermé)
    bool afficherCode(const std::string& code);
    bool effacerAffichage();
    bool demanderEnrolement(int idUser);

    // Arduino -> PC
    void lire();                                 // TODO: lire le port, puis recevoir()
    void recevoir(const std::string& octets);    // accumule, traite chaque ligne complète
    void simulerLigne(const std::string& ligne); // test sans Arduino

private:
    void traiterLigne(const std::string& ligne);
    bool envoyer(const std::string& commande);

    std::string m_tampon;
};