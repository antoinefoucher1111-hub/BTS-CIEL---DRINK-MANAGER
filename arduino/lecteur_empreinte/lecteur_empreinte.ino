#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> // Remplacez si vous utilisez un ecran OLED

// Configuration du capteur d'empreinte sur SoftwareSerial (Broches 2 et 3)
SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Configuration de l'afficheur LCD I2C (Adresse 0x27, 16 colonnes, 2 lignes)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Etats principaux du programme
enum SystemState {
  WAITING_FINGER,
  WAITING_CODE,
  ENROLL_MODE
};

SystemState currentState = WAITING_FINGER;

// Chronometre et duree (30 secondes)
unsigned long codeWaitStartTime = 0;
const unsigned long TIMEOUT_DURATION = 30000; // 30 000 ms = 30 secondes

void setup() {
  Serial.begin(9600);

  finger.begin(57600);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  if (finger.verifyPassword()) {
    lcd.print("En attente de");
    lcd.setCursor(0, 1);
    lcd.print("l'empreinte...");
  } else {
    lcd.print("Erreur Capteur");
    while (1) { delay(1); }
  }
}

void loop() {
  // 1. GESTION DES COMMANDES SERIE GLOBALES (ex: basculer en mode enregistrement avec 'e')
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("e")) {
      if (currentState != ENROLL_MODE) {
        currentState = ENROLL_MODE;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Mode Enreg.");
        lcd.setCursor(0, 1);
        lcd.print("Entrer ID ds Serie");
        Serial.println("--- MODE ENREGISTREMENT ACTIVE ---");
        Serial.println("Entrez le numero d'ID a attribuer (ex: 1, 2, 3...):");
      } else {
        // Si on etait deja en mode enregistrement, on en sort
        currentState = WAITING_FINGER;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("En attente de");
        lcd.setCursor(0, 1);
        lcd.print("l'empreinte...");
        Serial.println("--- RETOUR EN MODE NORMAL ---");
      }
      return; // On sort de la boucle pour traiter l'etat proprement
    }

    // Si on est en mode enregistrement et qu'on attend l'ID saisi dans le moniteur serie
    if (currentState == ENROLL_MODE) {
      int idToEnroll = input.toInt();
      if (idToEnroll > 0) {
        Serial.print("Enregistrement de l'ID : ");
        Serial.println(idToEnroll);

        // Lancement de la procedure d'enregistrement
        enrollFingerprint(idToEnroll);

        // Une fois termine, on repasse en mode normal
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("En attente de");
        lcd.setCursor(0, 1);
        lcd.print("l'empreinte...");
        currentState = WAITING_FINGER;
      } else {
        Serial.println("ID invalide. Entrez un nombre entier positif :");
      }
      return;
    }

    // Si on attend le code du Raspberry Pi (dans l'etat WAITING_CODE)
    if (currentState == WAITING_CODE) {
      if (input.length() == 6) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Code recu :");
        lcd.setCursor(0, 1);
        lcd.print(input);

        // On reinitialise le timer pour laisser le temps de lire le code
        codeWaitStartTime = millis();
      }
    }
  }

  // 2. MACHINE A ETATS (Fonctionnement normal)
  switch (currentState) {

    case WAITING_FINGER: {
      int fingerID = getFingerprintID();
      if (fingerID >= 0) {
        // Envoi discret de l'ID au Raspberry Pi (reste prive)
        Serial.println(fingerID);

        // Lancement du minuteur de 30 secondes et affichage "En attente du code..."
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("En attente du");
        lcd.setCursor(0, 1);
        lcd.print("code...");

        codeWaitStartTime = millis();
        currentState = WAITING_CODE;
      }
      break;
    }

    case WAITING_CODE: {
      // Dans tous les cas, au bout de 30 secondes, on revient a l'attente d'empreinte
      if (millis() - codeWaitStartTime >= TIMEOUT_DURATION) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("En attente de");
        lcd.setCursor(0, 1);
        lcd.print("l'empreinte...");

        currentState = WAITING_FINGER;
      }
      break;
    }

    case ENROLL_MODE: {
      // Rien a faire ici en boucle, on attend que l'utilisateur tape l'ID dans le Serial
      break;
    }
  }

  delay(50);
}

// Fonction pour identifier l'empreinte
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  return finger.fingerID;
}

// Fonction d'enregistrement modifiee avec les messages demandes
uint8_t enrollFingerprint(uint8_t id) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Placez votre");
  lcd.setCursor(0, 1);
  lcd.print("doigt...");
  Serial.println("Placez votre doigt sur le capteur...");

  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Erreur de conversion de l'image");
    return p;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Retirez le");
  lcd.setCursor(0, 1);
  lcd.print("doigt...");
  Serial.println("Retirez le doigt...");
  delay(2000);

  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Remettez le meme");
  lcd.setCursor(0, 1);
  lcd.print("doigt...");
  Serial.println("Remettez le meme doigt...");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Erreur de conversion de la 2eme image");
    return p;
  }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Erreur de creation du modele");
    return p;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Doigt");
    lcd.setCursor(0, 1);
    lcd.print("enregistre !");
    Serial.println("Succes : Doigt enregistre avec succes !");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erreur");
    lcd.setCursor(0, 1);
    lcd.print("enregistrement");
    Serial.println("Erreur lors de l'enregistrement dans la memoire.");
  }
  return p;
}
