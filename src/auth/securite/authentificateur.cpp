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
            m_jetons.emplace(jeton.codeHash, jeton);
            return true;
        }
    }
    return false;
}

ResultatVerif Authentificateur::verifier(const std::string& codeSaisi, std::time_t maintenant) {
    std::string hash = m_securite.hasher(codeSaisi);
    auto it = m_jetons.find(hash);
    if (it == m_jetons.end()) {
        return ResultatVerif::CODE_FAUX;
    }
    Jeton& j = it->second;
    if (maintenant < j.expiresAt) {
        // jeton valide
        return ResultatVerif::OK;
    }
    m_jetons.erase(hash); // doit-on supprimer après validation ? usage unique ? à copier avant OK si oui.
    return ResultatVerif::EXPIRE_OU_BLOQUE;
}