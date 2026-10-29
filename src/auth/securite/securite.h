#pragma once
#include <string>

class Securite
{
public:
    static std::string hasher(const std::string& texte);
    static std::string genererCode(); 
};