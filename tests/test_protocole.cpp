#include <iostream>
#include <string>
#include "../src/materiel/protocole.h"

static int echecs = 0;
#define VERIFIER(cond) do { if (!(cond)) { ++echecs; \
    std::cerr << "ECHEC ligne " << __LINE__ << " : " #cond "\n"; } } while (0)

static void valide(const std::string& in, Protocole::Type t, int id) {
    auto m = Protocole::analyser(in);
    VERIFIER(m.type == t);
    VERIFIER(m.id == id);
}
static void invalide(const std::string& in) {
    VERIFIER(Protocole::analyser(in).type == Protocole::Type::Invalide);
}

int main() {
    using T = Protocole::Type;

    // Arduino -> PC
    valide("USER:42", T::UserOk, 42);
    valide("USER:127", T::UserOk, Protocole::ID_MAX);
    valide("USER:42\r\n", T::UserOk, 42);
    valide("AUTH_FAIL", T::AuthFail, 0);
    valide("ENROLL_OK:7", T::EnrollOk, 7);
    valide("ENROLL_FAIL", T::EnrollFail, 0);

    for (const char* s : { "USER:abc", "USER:", "USER:-5", "USER:0", "USER:128",
                           "USER:4 2", "user:42", "" })
        invalide(s);
    invalide("USER:" + std::string(5000, '1'));

    // PC -> Arduino
    VERIFIER(Protocole::commandeCode("483920") == "CODE:483920\n");
    for (const char* c : { "", "12345", "1234567", "12a456", "12 456" })
        VERIFIER(Protocole::commandeCode(c).empty());
    VERIFIER(Protocole::commandeClear() == "CLEAR\n");
    VERIFIER(Protocole::commandeEnroll(42) == "ENROLL:42\n");
    VERIFIER(Protocole::commandeEnroll(Protocole::ID_MAX) == "ENROLL:127\n");
    VERIFIER(Protocole::commandeEnroll(0).empty());
    VERIFIER(Protocole::commandeEnroll(128).empty());

    std::cout << (echecs == 0 ? "Tous les tests passent\n" : "Des tests echouent\n");
    return echecs == 0 ? 0 : 1;
}