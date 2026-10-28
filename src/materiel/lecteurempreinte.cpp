#include "lecteurempreinte.h"
#include "protocole.h"

#include <cerrno>
#include <cstring>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {
void signalerErreur(const std::function<void(const std::string&)>& callback,
                    const std::string& message)
{
    if (callback) callback(message);
}

#ifndef _WIN32
bool vitesseSerie(int baud, speed_t& vitesse)
{
    switch (baud) {
    case 50: vitesse = B50; return true;
    case 75: vitesse = B75; return true;
    case 110: vitesse = B110; return true;
    case 134: vitesse = B134; return true;
    case 150: vitesse = B150; return true;
    case 200: vitesse = B200; return true;
    case 300: vitesse = B300; return true;
    case 600: vitesse = B600; return true;
    case 1200: vitesse = B1200; return true;
    case 1800: vitesse = B1800; return true;
    case 2400: vitesse = B2400; return true;
    case 4800: vitesse = B4800; return true;
    case 9600: vitesse = B9600; return true;
    case 19200: vitesse = B19200; return true;
    case 38400: vitesse = B38400; return true;
    case 57600: vitesse = B57600; return true;
    case 115200: vitesse = B115200; return true;
#ifdef B230400
    case 230400: vitesse = B230400; return true;
#endif
    default: return false;
    }
}
#endif
}

LecteurEmpreinte::~LecteurEmpreinte()
{
    fermer();
}

bool LecteurEmpreinte::ouvrir(const std::string& nomPort, int baud)
{
    fermer();
    if (nomPort.empty() || baud <= 0) {
        signalerErreur(surErreur, "Nom de port ou débit série invalide");
        return false;
    }

#ifdef _WIN32
    const std::string chemin = nomPort.rfind("\\\\.\\", 0) == 0
        ? nomPort
        : "\\\\.\\" + nomPort;
    HANDLE port = CreateFileA(chemin.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                              nullptr, OPEN_EXISTING, 0, nullptr);
    if (port == INVALID_HANDLE_VALUE) {
        signalerErreur(surErreur, "Impossible d'ouvrir le port série (erreur "
                       + std::to_string(GetLastError()) + ")");
        return false;
    }

    DCB configuration = {};
    configuration.DCBlength = sizeof(configuration);
    if (!GetCommState(port, &configuration)) {
        const DWORD erreur = GetLastError();
        CloseHandle(port);
        signalerErreur(surErreur, "Impossible de lire la configuration série (erreur "
                       + std::to_string(erreur) + ")");
        return false;
    }
    configuration.BaudRate = static_cast<DWORD>(baud);
    configuration.ByteSize = 8;
    configuration.Parity = NOPARITY;
    configuration.StopBits = ONESTOPBIT;
    configuration.fBinary = TRUE;
    configuration.fParity = FALSE;
    configuration.fOutxCtsFlow = FALSE;
    configuration.fOutxDsrFlow = FALSE;
    configuration.fDtrControl = DTR_CONTROL_ENABLE;
    configuration.fDsrSensitivity = FALSE;
    configuration.fTXContinueOnXoff = TRUE;
    configuration.fOutX = FALSE;
    configuration.fInX = FALSE;
    configuration.fRtsControl = RTS_CONTROL_ENABLE;
    configuration.fAbortOnError = FALSE;

    if (!SetCommState(port, &configuration)) {
        const DWORD erreur = GetLastError();
        CloseHandle(port);
        signalerErreur(surErreur, "Impossible de configurer le port série (erreur "
                       + std::to_string(erreur) + ")");
        return false;
    }

    COMMTIMEOUTS delais = {};
    delais.ReadIntervalTimeout = MAXDWORD;
    delais.WriteTotalTimeoutConstant = 1000;
    if (!SetCommTimeouts(port, &delais)) {
        const DWORD erreur = GetLastError();
        CloseHandle(port);
        signalerErreur(surErreur, "Impossible de configurer les délais série (erreur "
                       + std::to_string(erreur) + ")");
        return false;
    }
    if (!SetupComm(port, 4096, 4096) || !PurgeComm(port, PURGE_RXCLEAR | PURGE_TXCLEAR)) {
        const DWORD erreur = GetLastError();
        CloseHandle(port);
        signalerErreur(surErreur, "Impossible de préparer le port série (erreur "
                       + std::to_string(erreur) + ")");
        return false;
    }
    m_port = reinterpret_cast<std::intptr_t>(port);
#else
    speed_t vitesse;
    if (!vitesseSerie(baud, vitesse)) {
        signalerErreur(surErreur, "Débit série non pris en charge");
        return false;
    }

    const int port = open(nomPort.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (port == -1) {
        signalerErreur(surErreur, "Impossible d'ouvrir le port série : "
                       + std::string(strerror(errno)));
        return false;
    }

    termios configuration = {};
    if (tcgetattr(port, &configuration) == -1) {
        const int erreur = errno;
        close(port);
        signalerErreur(surErreur, "Impossible de lire la configuration série : "
                       + std::string(strerror(erreur)));
        return false;
    }

    configuration.c_iflag = IGNBRK;
    configuration.c_oflag = 0;
    configuration.c_lflag = 0;
    configuration.c_cflag &= ~CSIZE;
    configuration.c_cflag |= CS8 | CLOCAL | CREAD;
    configuration.c_cflag &= ~(PARENB | CSTOPB);
#ifdef CRTSCTS
    configuration.c_cflag &= ~CRTSCTS;
#endif
    configuration.c_cc[VMIN] = 0;
    configuration.c_cc[VTIME] = 0;

    if (cfsetispeed(&configuration, vitesse) == -1
        || cfsetospeed(&configuration, vitesse) == -1
        || tcsetattr(port, TCSANOW, &configuration) == -1) {
        const int erreur = errno;
        close(port);
        signalerErreur(surErreur, "Impossible de configurer le port série : "
                       + std::string(strerror(erreur)));
        return false;
    }
    if (tcflush(port, TCIOFLUSH) == -1) {
        const int erreur = errno;
        close(port);
        signalerErreur(surErreur, "Impossible de vider les tampons série : "
                       + std::string(strerror(erreur)));
        return false;
    }
    m_port = port;
#endif
    return true;
}

void LecteurEmpreinte::fermer()
{
    if (m_port != -1) {
#ifdef _WIN32
        if (!CloseHandle(reinterpret_cast<HANDLE>(m_port))) {
            signalerErreur(surErreur, "Impossible de fermer le port série (erreur "
                           + std::to_string(GetLastError()) + ")");
        }
#else
        if (close(static_cast<int>(m_port)) == -1) {
            const int erreur = errno;
            signalerErreur(surErreur, "Impossible de fermer le port série : "
                           + std::string(strerror(erreur)));
        }
#endif
    }
    m_port = -1;
    m_tampon.clear();
    m_rejeterLigneTropLongue = false;
}

void LecteurEmpreinte::lire()
{
    if (m_port == -1) return;

#ifdef _WIN32
    HANDLE port = reinterpret_cast<HANDLE>(m_port);
    DWORD erreurs = 0;
    COMSTAT etat = {};
    if (!ClearCommError(port, &erreurs, &etat)) {
        signalerErreur(surErreur, "Erreur de lecture du port série (erreur "
                       + std::to_string(GetLastError()) + ")");
        return;
    }
    if (etat.cbInQue == 0) return;

    std::string octets(etat.cbInQue, '\0');
    DWORD lus = 0;
    if (!ReadFile(port, &octets[0], etat.cbInQue, &lus, nullptr)) {
        signalerErreur(surErreur, "Impossible de lire le port série (erreur "
                       + std::to_string(GetLastError()) + ")");
        return;
    }
    octets.resize(lus);
    recevoir(octets);
#else
    char octets[256];
    for (;;) {
        const ssize_t lus = read(static_cast<int>(m_port), octets, sizeof(octets));
        if (lus > 0) {
            recevoir(std::string(octets, static_cast<std::size_t>(lus)));
            continue;
        }
        if (lus == 0 || errno == EAGAIN || errno == EWOULDBLOCK) return;
        if (errno == EINTR) continue;
        signalerErreur(surErreur, "Impossible de lire le port série : "
                       + std::string(strerror(errno)));
        return;
    }
#endif
}

bool LecteurEmpreinte::envoyer(const std::string& commande)
{
    if (commande.empty() || m_port == -1) return false;

#ifdef _WIN32
    HANDLE port = reinterpret_cast<HANDLE>(m_port);
    std::size_t position = 0;
    while (position < commande.size()) {
        const DWORD taille = static_cast<DWORD>(commande.size() - position);
        DWORD ecrits = 0;
        if (!WriteFile(port, commande.data() + position, taille, &ecrits, nullptr)
            || ecrits == 0) {
            signalerErreur(surErreur, "Impossible d'écrire sur le port série (erreur "
                           + std::to_string(GetLastError()) + ")");
            return false;
        }
        position += ecrits;
    }
#else
    const int port = static_cast<int>(m_port);
    std::size_t position = 0;
    while (position < commande.size()) {
        const ssize_t ecrits = write(port, commande.data() + position,
                                     commande.size() - position);
        if (ecrits > 0) {
            position += static_cast<std::size_t>(ecrits);
            continue;
        }
        if (ecrits == -1 && errno == EINTR) continue;
        if (ecrits == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd attente = { port, POLLOUT, 0 };
            const int resultat = poll(&attente, 1, 1000);
            if (resultat > 0 && (attente.revents & POLLOUT)) continue;
            if (resultat == 0) {
                signalerErreur(surErreur, "Délai dépassé lors de l'écriture sur le port série");
            } else if (resultat == -1) {
                signalerErreur(surErreur, "Impossible d'attendre le port série : "
                               + std::string(strerror(errno)));
            } else {
                signalerErreur(surErreur, "Le port série n'est pas disponible pour l'écriture");
            }
            return false;
        }
        signalerErreur(surErreur, "Impossible d'écrire sur le port série : "
                       + std::string(strerror(errno)));
        return false;
    }
#endif
    return true;
}

bool LecteurEmpreinte::afficherCode(const std::string& code)
{
    return envoyer(Protocole::commandeCode(code));
}

bool LecteurEmpreinte::effacerAffichage()
{
    return envoyer(Protocole::commandeClear());
}

bool LecteurEmpreinte::demanderEnrolement(int idUser)
{
    return envoyer(Protocole::commandeEnroll(idUser));
}

void LecteurEmpreinte::recevoir(const std::string& octets)
{
    for (char octet : octets) {
        if (octet == '\n') {
            if (m_rejeterLigneTropLongue) {
                signalerErreur(surErreur, "Ligne reçue trop longue");
                m_rejeterLigneTropLongue = false;
            } else {
                traiterLigne(m_tampon);
            }
            m_tampon.clear();
            continue;
        }

        if (m_rejeterLigneTropLongue) continue;
        if (m_tampon.size() >= Protocole::LONGUEUR_MAX + 2) {
            m_tampon.clear();
            m_rejeterLigneTropLongue = true;
            continue;
        }
        m_tampon += octet;
    }
}

void LecteurEmpreinte::simulerLigne(const std::string& ligne)
{
    traiterLigne(ligne);
}

void LecteurEmpreinte::traiterLigne(const std::string& ligne)
{
    const Protocole::Message m = Protocole::analyser(ligne);
    switch (m.type) {
    case Protocole::Type::UserOk:
        if (surUtilisateurIdentifie) surUtilisateurIdentifie(m.id);
        break;
    case Protocole::Type::AuthFail:
        if (surAuthEchouee) surAuthEchouee();
        break;
    case Protocole::Type::EnrollOk:
        if (surEnrolement) surEnrolement(true, m.id);
        break;
    case Protocole::Type::EnrollFail:
        if (surEnrolement) surEnrolement(false, 0);
        break;
    case Protocole::Type::Invalide:
        if (surErreur) surErreur("Ligne invalide : " + ligne.substr(0, 40));
        break;
    }
}