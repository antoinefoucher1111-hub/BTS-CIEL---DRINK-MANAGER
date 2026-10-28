#include <QtTest>
#include "../src/materiel/protocole.h"

class TestProtocole : public QObject {
    Q_OBJECT
private slots:
    void valides_data() {
        QTest::addColumn<QString>("entree");
        QTest::addColumn<int>("type");
        QTest::addColumn<int>("id");
        QTest::newRow("user")        << "USER:42"        << int(Protocole::Type::UserOk)   << 42;
        QTest::newRow("user crlf")   << "USER:42\r\n"    << int(Protocole::Type::UserOk)   << 42;
        QTest::newRow("auth fail")   << "AUTH_FAIL"      << int(Protocole::Type::AuthFail) << 0;
        QTest::newRow("enroll ok")   << "ENROLL_OK:7"    << int(Protocole::Type::EnrollOk) << 7;
        QTest::newRow("enroll fail") << "ENROLL_FAIL"    << int(Protocole::Type::EnrollFail) << 0;
    }
    void valides() {
        QFETCH(QString, entree); QFETCH(int, type); QFETCH(int, id);
        const auto m = Protocole::analyser(entree);
        QCOMPARE(int(m.type), type);
        QCOMPARE(m.id, id);
    }

    void invalides_data() {
        QTest::addColumn<QString>("entree");
        QTest::newRow("lettres")   << "USER:abc";
        QTest::newRow("vide id")   << "USER:";
        QTest::newRow("negatif")   << "USER:-5";
        QTest::newRow("zero")      << "USER:0";
        QTest::newRow("trop grand")<< "USER:128";
        QTest::newRow("espace")    << "USER:4 2";
        QTest::newRow("minuscule") << "user:42";
        QTest::newRow("ligne vide")<< "";
        QTest::newRow("5000 car.") << ("USER:" + QString(5000, '1'));
    }
    void invalides() {
        QFETCH(QString, entree);
        QCOMPARE(Protocole::analyser(entree).type, Protocole::Type::Invalide);
    }
};

QTEST_APPLESS_MAIN(TestProtocole)
#include "test_protocole.moc"