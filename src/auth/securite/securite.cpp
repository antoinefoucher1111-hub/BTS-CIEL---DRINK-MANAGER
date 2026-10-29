#include "securite.h"
#include "picosha2.h"
#include <random>
#include <sstream>
#include <iomanip>

std::string Securite::hasher(const std::string& texte)
{
    return picosha2::hash256_hex_string(texte);
}

std::string Securite::genererCode()
{
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 999999);
    int valeur = dist(rd);
    std::ostringstream flux;
    flux << std::setfill('0') << std::setw(6) << valeur;
    return flux.str();
}