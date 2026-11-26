# Tutoriel Complet A à Z : Déployer Drink Manager sur Raspberry Pi 5

Ce guide explique **ce qu'on installe sur le Raspberry Pi**, comment relier le **capteur biométrique AS608 + Arduino UNO** au **backend C++ / SQLite**, et comment rendre l'**interface web** accessible sur le réseau local (et via QR Code).

---

## 1. Architecture globale du système

```text
[Capteur AS608] <--- UART (D2/D3) ---> [Arduino UNO] <--- USB Série (/dev/ttyACM0) ---> [Raspberry Pi 5]
                                                                                              |
                     +------------------------------------------------------------------------+
                     |
                     +--> 1. Pont Série (raspberry_serial_bridge.py)
                     |       Lit "USER:<id>" -> appelle POST /api/auth/biometric -> renvoie "CODE:XXXXXX\n"
                     |
                     +--> 2. Backend Natif C++17 (drink_manager) [Port 8080]
                     |       API REST + Protocole + ORM SQLite -> backend/data/drinkmanager.db
                     |
                     +--> 3. Interface Web (drink-manager-web) [Port 8081 ou 80]
                             Accessible sur PC / Smartphone (QR Code ?user_id=...)
```

### Ce qu'on met sur le Raspberry Pi
1. **Le dossier `backend/`** (code C++17 + `httplib.h` + `protocole.cpp` + base `data/drinkmanager.db` + `scripts/raspberry_serial_bridge.py`).
2. **Le dossier `drink-manager-web/`** (ou son build statique `dist/` servi en HTTP pour ne pas avoir besoin de lancer Metro/Expo en permanence).
3. **Les services `systemd`** pour démarrer automatiquement l'API C++ et le pont série au démarrage du Raspberry Pi.

---

## 2. Préparation du Raspberry Pi 5 (OS & Dépendances)

### 2.1 Flash de la carte SD
1. Télécharger **Raspberry Pi Imager** sur PC.
2. Choisir **Raspberry Pi OS (64-bit)** (version *Desktop* si vous branchez un écran tactile en mode borne, ou *Lite* en mode serveur).
3. Dans les paramètres avancés (*Edit Settings*) :
   - Définir le nom d'hôte : `drinkmanager` (accessible ensuite via `drinkmanager.local`).
   - Créer un utilisateur (ex: `pi`) et activer **SSH**.
   - Configurer le **Wi-Fi** (ou utiliser un câble Ethernet).

### 2.2 Connexion SSH et installation des outils
Connectez-vous au Raspberry Pi depuis votre PC :
```bash
ssh pi@drinkmanager.local
```

Mettez à jour le système et installez les dépendances nécessaires (compilateur C++, CMake, SQLite, Python série, Node.js) :
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y git build-essential cmake libsqlite3-dev sqlite3 python3-serial nodejs npm
```

Ajoutez votre utilisateur au groupe `dialout` pour accéder au port USB de l'Arduino sans `sudo` :
```bash
sudo usermod -aG dialout $USER
```
*(Déconnectez-vous puis reconnectez-vous en SSH pour que le groupe `dialout` soit pris en compte.)*

---

## 3. Raccordement matériel (AS608 + Arduino UNO -> Raspberry Pi)

### 3.1 Câblage AS608 <-> Arduino UNO
| Broche AS608 | Broche Arduino UNO | Rôle |
|---|---|---|
| **VCC (Rouge)** | `5V` | Alimentation |
| **GND (Noir)** | `GND` | Masse commune |
| **TX (Jaune/Blanc)** | `D2` | Réception côté Arduino (`SoftwareSerial` RX) |
| **RX (Vert/Bleu)** | `D3` | Émission côté Arduino (`SoftwareSerial` TX) |

### 3.2 Flash du code Arduino
1. Sur votre PC, ouvrez l'IDE Arduino et installez la bibliothèque **Adafruit Fingerprint Sensor Library**.
2. Enrôlez vos premières empreintes avec l'exemple `enroll` (IDs `1`, `2`, `3`... correspondant à la colonne `AccessPrint` de la table `Users`).
3. Téléversez le sketch final qui envoie `USER:<id>\n` lors d'une reconnaissance réussie et `AUTH_FAIL\n` en cas d'échec (baudrate `9600`), et qui lit `CODE:XXXXXX\n` / `CLEAR\n`.
4. Branchez l'Arduino UNO en **USB** sur le Raspberry Pi 5.

Vérifiez sur le Raspberry Pi que l'Arduino est bien détecté :
```bash
ls /dev/ttyACM* /dev/ttyUSB*
```
*(En général `/dev/ttyACM0` pour un Arduino UNO officiel, ou `/dev/ttyUSB0` pour un clone CH340).*

---

## 4. Déploiement & compilation du Backend C++ sur le Raspberry Pi

### 4.1 Copier le projet sur le Raspberry Pi
Depuis votre PC Windows (PowerShell), copiez `backend` et `drink-manager-web` sur le Raspberry Pi (sans `node_modules` ni `build`) :
```powershell
scp -r backend\src backend\include backend\sqlite backend\data backend\scripts backend\CMakeLists.txt pi@drinkmanager.local:~/drink-manager/backend/
```
*(Ou via `git clone` si vous avez poussé les fichiers sur votre dépôt GitHub.)*

### 4.2 Compiler le serveur C++ natif (ARM64 Linux)
Sur le Raspberry Pi :
```bash
cd ~/drink-manager/backend
mkdir -p build && cd build
cmake ..
make -j4
```
Le binaire `drink_manager` est généré dans `~/drink-manager/backend/build/drink_manager`.

### 4.3 Tester le serveur C++ et la BDD
```bash
cd ~/drink-manager/backend
./build/drink_manager
```
Vous devez voir :
```text
[INFO] Base data/drinkmanager.db ouverte avec succes.
[INFO] Serveur actif sur http://0.0.0.0:8080
```
Dans un second terminal SSH, testez l'API :
```bash
curl http://localhost:8080/api/health
curl http://localhost:8080/api/users/1
curl http://localhost:8080/api/drinks
```

---

## 5. Lancer le pont série Arduino <-> API C++

Dans un terminal sur le Raspberry Pi, lancez le script fourni dans `backend/scripts/raspberry_serial_bridge.py` :
```bash
python3 ~/drink-manager/backend/scripts/raspberry_serial_bridge.py --port /dev/ttyACM0 --baud 9600
```

Fonctionnement en temps réel :
1. L'utilisateur pose son doigt sur le capteur AS608.
2. L'Arduino envoie `USER:1\n` sur le port série.
3. Le script appelle `POST http://127.0.0.1:8080/api/auth/biometric` avec `{"serial_line": "USER:1"}`.
4. Le backend C++ vérifie `AccessPrint = 1` dans SQLite, génère un code à 6 chiffres (valable 2 minutes) et renvoie `CODE:483920\n`.
5. Le script envoie `CODE:483920\n` à l'Arduino pour l'afficher.
6. L'utilisateur saisit `483920` dans l'interface web (bouton 🔐) et est connecté automatiquement !

---

## 6. Déployer l'Interface Web sur le Raspberry Pi

### Option A : Mode développement rapide (Expo Web)
Copiez `drink-manager-web` sur le Pi puis :
```bash
cd ~/drink-manager/drink-manager-web
npm install
npx expo start --web --port 8081
```

### Option B : Mode production léger (recommandé pour le Raspberry Pi)
Générez le site statique une seule fois (sur votre PC ou sur le Pi) :
```bash
cd drink-manager-web
npx expo export --platform web
```
Copiez le dossier `dist/` généré sur le Raspberry Pi dans `~/drink-manager/web-dist/`, puis servez-le simplement avec `npx serve` ou `python3 -m http.server` (ou `nginx`) :
```bash
python3 -m http.server 8081 --directory ~/drink-manager/web-dist
```
Comme `api.ts` utilise `window.location.hostname`, l'interface web contactera automatiquement `http://<IP_DU_RASPBERRY>:8080/api` depuis n'importe quel téléphone ou PC du réseau !

- **Accès direct via QR Code** :
  Générez un QR Code pointant vers `http://drinkmanager.local:8081` (ou `http://<IP_RASPBERRY>:8081`).
  **La connexion est obligatoire** : l'utilisateur pose son doigt sur le capteur, puis saisit le code à 6 chiffres affiché. Aucun accès par `?user_id=` n'est possible.

  Sécurité côté serveur : sans jeton de session (obtenu via le code), toutes les routes `/api` (sauf `/api/health`) renvoient **401** ; un utilisateur ne peut lire/modifier que **son** compte (**403** sinon). `/api/auth/biometric`, `POST /api/users` et `PUT /api/users/:id/fingerprint` ne sont acceptés que depuis le Raspberry Pi lui-même (localhost).

  Test sans Arduino : lancer le serveur avec `DM_ALLOW_SIM=1 ./build/drink_manager` pour activer le simulateur (à ne jamais laisser en production).

---

## 7. Démarrage automatique (`systemd`)

Pour que la borne fonctionne toute seule dès qu'on branche le Raspberry Pi :

### 7.1 Service Backend C++ (`/etc/systemd/system/drink-manager-api.service`)
```ini
[Unit]
Description=Drink Manager C++ API Server
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/drink-manager/backend
ExecStart=/home/pi/drink-manager/backend/build/drink_manager
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
```

### 7.2 Service Pont Série (`/etc/systemd/system/drink-manager-serial.service`)
```ini
[Unit]
Description=Drink Manager Arduino Serial Bridge
After=drink-manager-api.service

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/drink-manager/backend
ExecStart=/usr/bin/python3 /home/pi/drink-manager/backend/scripts/raspberry_serial_bridge.py --port /dev/ttyACM0
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
```

### 7.3 Activation des services
```bash
sudo systemctl daemon-reload
sudo systemctl enable drink-manager-api drink-manager-serial
sudo systemctl start drink-manager-api drink-manager-serial
```
Pour consulter les logs en direct :
```bash
journalctl -u drink-manager-api -u drink-manager-serial -f
```


