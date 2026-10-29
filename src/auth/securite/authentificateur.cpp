#include "authentificateur.h"

bool Authentificateur::surUtilisateurIdentifie(int fingerprintId, std::time_t maintenant) {
    /* c++ 21
    if (m_connus.contains(fingerprintId)) {
        return true
    }
    */
    for (int fid : m_empreintes_connus) {
        if (fid == fingerprintId) {
            std::string code = m_securite.genererCode();
            Jeton jeton = m_gestion_jeton.creer(fid, code, maintenant);
            m_afficheur.afficher(code);
            m_jetons.emplace(jeton);
            return true;
        }
    }
    return false;
}

ResultatVerif Authentificateur::verifier(int fingerprintId, const std::string& codeSaisi, std::time_t maintenant) {

}