#include "securite.h"
#include "picosha2.h"

std::string Securite::hasher(const std::string& texte)
{
    return picosha2::hash256_hex_string(texte);
}

