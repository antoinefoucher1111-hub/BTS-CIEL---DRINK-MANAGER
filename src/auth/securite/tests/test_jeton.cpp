#include <iostream>
#include "jeton.h"
#include "securite.h"

int main()
{
    std::time_t t = 1000;

    // ----- Partie 1 : estValide -----
    std::cout << "--- estValide ---" << std::endl;
    Jeton j = GestionJeton::creer(42, "123456", t);
    std::cout << "t=1000 (attendu 1)  : " << GestionJeton::estValide(j, 1000) << std::endl;
    std::cout << "t=1059 (attendu 1)  : " << GestionJeton::estValide(j, 1059) << std::endl;
    std::cout << "t=1060 (attendu 0)  : " << GestionJeton::estValide(j, 1060) << std::endl;

    j.used = true;
    std::cout << "used   (attendu 0)  : " << GestionJeton::estValide(j, t) << std::endl;

    j.used = false;
    j.tentatives = 3;
    std::cout << "3 essais (attendu 0): " << GestionJeton::estValide(j, t) << std::endl;

    // ----- Partie 2 : verifierCode (0=OK, 1=CODE_FAUX, 2=EXPIRE_OU_BLOQUE) -----
    std::cout << "--- verifierCode ---" << std::endl;

    Jeton a = GestionJeton::creer(42, "123456", t);
    std::cout << "1 bon code (attendu 0)      : "
              << static_cast<int>(GestionJeton::verifierCode(a, "123456", t)) << std::endl;
    std::cout << "2 meme code (attendu 2)     : "
              << static_cast<int>(GestionJeton::verifierCode(a, "123456", t)) << std::endl;

    Jeton b = GestionJeton::creer(42, "123456", t);
    std::cout << "3 faux code (attendu 1)     : "
              << static_cast<int>(GestionJeton::verifierCode(b, "000000", t))
              << " tentatives=" << b.tentatives << " (attendu 1)" << std::endl;

    Jeton c = GestionJeton::creer(42, "123456", t);
    GestionJeton::verifierCode(c, "000001", t);
    GestionJeton::verifierCode(c, "000002", t);
    GestionJeton::verifierCode(c, "000003", t);
    std::cout << "4 bon apres 3 echecs (att 2): "
              << static_cast<int>(GestionJeton::verifierCode(c, "123456", t)) << std::endl;

    Jeton d = GestionJeton::creer(42, "123456", t);
    std::cout << "5 expire (attendu 2)        : "
              << static_cast<int>(GestionJeton::verifierCode(d, "123456", t + 60)) << std::endl;

    return 0;
}