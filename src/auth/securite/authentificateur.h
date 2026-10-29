#pragma once
#include <map>
#include <set>
#include <string>
#include <ctime>
#include "jeton.h"
#include "securite.h"
#include "../materiel/afficheur.h"

class Authentificateur
{
    // Attributs
    private:
        IAfficheur&          m_afficheur;
        GestionJeton         m_gestion_jeton;
        Securite             m_securite;
        std::set<int>        m_empreintes_connus;
        std::map<int, Jeton> m_jetons;

    // Methodes
    public:
        Authentificateur(IAfficheur& afficheur, const std::set<int>& empreintesConnues, const Securite securite, const GestionJeton gestionJeton) :
            m_afficheur(afficheur), m_empreintes_connus(empreintesConnues), m_securite(securite), m_gestion_jeton(gestionJeton) {}

        bool surUtilisateurIdentifie(int fingerprintId, std::time_t maintenant);
        ResultatVerif verifier(int fingerprintId, const std::string& codeSaisi, std::time_t maintenant);
};