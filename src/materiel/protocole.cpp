#include "protocole.h"

Protocole::Message Protocole::analyser(const QString& brut)
{
    const QString ligne = brut.trimmed();   // gère \r\n et \n
    if (ligne.isEmpty() || ligne.size() > LONGUEUR_MAX)
        return {};

    if (ligne == "AUTH_FAIL")
        return { Type::AuthFail, 0 };

    if (ligne == "ENROLL_FAIL")
        return { Type::EnrollFail, 0 };

    // USER:<id> et ENROLL_OK:<id> ont la même forme
    struct Prefixe { QString texte; Type type; };
    const Prefixe prefixes[] = {
        { "USER:",      Type::UserOk },
        { "ENROLL_OK:", Type::EnrollOk },
    };
    for (const auto& p : prefixes) {
        if (ligne.startsWith(p.texte)) {
            bool ok = false;
            const int id = ligne.mid(p.texte.size()).toInt(&ok);
            if (ok && id >= 1 && id <= ID_MAX)
                return { p.type, id };
            return {};
        }
    }
    return {};
}