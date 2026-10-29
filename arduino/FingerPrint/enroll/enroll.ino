#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

// Création d'une classe C++ pour encapsuler la logique du capteur
class BiometricScanner {
  private:
    SoftwareSerial mySerial;
    Adafruit_Fingerprint finger;

  public:
    // Constructeur : initialise la communication série sur les broches choisies
    BiometricScanner(uint8_t rxPin, uint8_t txPin) 
      : mySerial(rxPin, txPin), finger(&mySerial) {}

    void begin() {
      Serial.begin(9600);
      while (!Serial); 
      delay(100);
      
      Serial.println("\n\nInitialisation du module biometrique...");

      // Vitesse par défaut du capteur
      finger.begin(57600);
      
      if (finger.verifyPassword()) {
        Serial.println("Capteur d'empreinte detecte et operationnel !");
      } else {
        Serial.println("Erreur : Capteur introuvable. Verifiez le cablage.");
        while (1) { delay(1); } // Bloque l'exécution en cas d'erreur matérielle
      }
    }

    void runEnrollment() {
      Serial.println("Veuillez entrer un numero d'ID (de 1 a 127) pour enregistrer une empreinte : ");
      uint8_t id = readNumber();
      if (id == 0) return;
      
      Serial.print("Debut de la procedure pour l'ID #");
      Serial.println(id);
      
      while (!enrollFinger(id));
    }

  private:
    uint8_t readNumber() {
      uint8_t num = 0;
      while (num == 0) {
        while (!Serial.available());
        num = Serial.parseInt();
      }
      return num;
    }

    bool enrollFinger(uint8_t id) {
      int p = -1;
      Serial.print("Attente d'un doigt valide pour l'ID #"); Serial.println(id);
      
      // Etape 1 : Prendre la première image
      while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        if (p == FINGERPRINT_OK) {
          Serial.println("Image capturee");
        }
      }

      p = finger.image2Tz(1);
      if (p != FINGERPRINT_OK) {
        Serial.println("Erreur de conversion de l'image");
        return false;
      }

      Serial.println("Retirez le doigt");
      delay(2000);
      p = 0;
      while (p != FINGERPRINT_NOFINGER) {
        p = finger.getImage();
      }
      
      Serial.println("Placez le MEME doigt a nouveau");
      p = -1;
      while (p != FINGERPRINT_OK) {
        p = finger.getImage();
      }

      // Etape 2 : Prendre la seconde image pour confirmer
      p = finger.image2Tz(2);
      if (p != FINGERPRINT_OK) {
         Serial.println("Erreur de conversion de l'image");
         return false;
      }

      // Etape 3 : Créer et stocker le modèle mathématique
      p = finger.createModel();
      if (p == FINGERPRINT_ENROLLMISMATCH) {
        Serial.println("Les deux empreintes ne correspondent pas");
        return false;
      } else if (p != FINGERPRINT_OK) {
        Serial.println("Erreur de creation du modele");
        return false;
      }

      p = finger.storeModel(id);
      if (p == FINGERPRINT_OK) {
        Serial.println("Empreinte enregistree avec succes !");
        return true;
      } else {
        Serial.println("Erreur lors de l'enregistrement dans la memoire");
        return false;
      }
    }
};

// --- Structure standard Arduino ---

// Instanciation de l'objet (TX capteur -> RX Arduino 2, RX capteur -> TX Arduino 3)
BiometricScanner sasFingerprint(2, 3);

void setup() {
  sasFingerprint.begin();
}

void loop() {
  sasFingerprint.runEnrollment();
}