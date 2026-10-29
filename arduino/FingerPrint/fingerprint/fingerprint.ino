#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

class BiometricScanner {
  private:
    SoftwareSerial mySerial;
    Adafruit_Fingerprint finger;

  public:
    // Initialisation avec les mêmes broches que pour l'enregistrement (RX Arduino 2, TX Arduino 3)
    BiometricScanner(uint8_t rxPin, uint8_t txPin) 
      : mySerial(rxPin, txPin), finger(&mySerial) {}

    void begin() {
      Serial.begin(9600);
      while (!Serial); 
      delay(100);
      Serial.println("\n--- Demarrage du systeme d'authentification ---");

      finger.begin(57600);
      delay(5);
      
      if (finger.verifyPassword()) {
        Serial.println("Capteur d'empreinte detecte et pret.");
      } else {
        Serial.println("Erreur : Capteur introuvable. Verifiez les branchements.");
        while (1) { delay(1); } // Stoppe l'exécution
      }

      finger.getTemplateCount();
      if (finger.templateCount == 0) {
        Serial.println("Attention : Aucune empreinte dans la memoire !");
      } else {
        Serial.print("Base de donnees : ");
        Serial.print(finger.templateCount);
        Serial.println(" empreintes enregistrees.");
        Serial.println("En attente d'une lecture...");
      }
    }

    // Fonction principale qui gère le scan et la recherche
    int scanFinger() {
      uint8_t p = finger.getImage();
      if (p != FINGERPRINT_OK) return -1; // Pas de doigt ou erreur de capture

      p = finger.image2Tz();
      if (p != FINGERPRINT_OK) {
        Serial.println("Erreur lors de la conversion de l'image.");
        return -1;
      }

      p = finger.fingerSearch();
      if (p == FINGERPRINT_OK) {
        // Correspondance trouvée !
        return finger.fingerID;
      } else if (p == FINGERPRINT_NOTFOUND) {
        Serial.println("Empreinte non reconnue (Acces refuse).");
        return -1;
      } else {
        Serial.println("Erreur de communication lors de la recherche.");
        return -1;
      }
    }
};

// Instanciation du scanner pour le sas
BiometricScanner sasFingerprint(2, 3);

void setup() {
  sasFingerprint.begin();
}

void loop() {
  int id = sasFingerprint.scanFinger();
  
  if (id != -1) {
    // Si l'ID est valide (différent de -1), on déclenche l'action
    Serial.print("========== ACCES AUTORISE ==========\n");
    Serial.print("Bienvenue, Utilisateur ID #");
    Serial.println(id);
    Serial.println("====================================");
    
    // Insérer ici le code pour ouvrir la porte (ex: activer un relais, communiquer avec l'ESP32)
    
    delay(3000); // Pause de 3 secondes avant d'autoriser un nouveau scan
    Serial.println("\nEn attente d'une nouvelle lecture...");
  }
  
  delay(50); // Petite pause pour ne pas saturer le processeur
}