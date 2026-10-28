#pragma once
#include <QObject>
#include <QSerialPort>
#include <QByteArray>

class LecteurEmpreinte : public QObject {
    Q_OBJECT
public:
    explicit LecteurEmpreinte(QObject* parent = nullptr);

    bool ouvrir(const QString& nomPort, qint32 baud = 9600);
    void fermer();

    // PC -> Arduino
    void envoyerLedOk();
    void envoyerBuzzer();
    void demanderEnrolement(int idUser);

    // Pour tester sans matériel (appelé par le bouton du simulateur)
    void simulerLigne(const QString& ligne);

signals:
    void utilisateurIdentifie(int idUser);
    void authEchouee();
    void erreurPort(const QString& message);

private slots:
    void surDonneesRecues();

private:
    void traiterLigne(const QString& ligne);
    void envoyer(const QByteArray& commande);

    QSerialPort m_port;
    QByteArray  m_tampon;   // accumule les octets jusqu'au '\n'
};