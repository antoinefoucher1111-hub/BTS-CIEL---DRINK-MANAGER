# Notes de Yohan — Personne A (côté matériel)

## Fait
 
| Élément | Fichier | État |
|---|---|---|
| Analyse des messages série | `src/materiel/protocole.h/.cpp` | Terminé, testé |
| Tests | `tests/test_protocole.cpp` | Tous passent |
| Interface du lecteur | `src/materiel/lecteurempreinte.h` | Écrite |
| Implémentation du lecteur, code Arduino, simulateur | `lecteurempreinte.cpp`, `.ino` | À compléter |
 
## Décisions
 
- `idUser` = `fingerprintId` (numéro d'emplacement du capteur, de 1 à `ID_MAX` = 127).
- Plus de Qt : C++ standard + `g++`. `QString` devient `std::string`, les signaux deviennent des callbacks `std::function`.
- Pas de LED ni de buzzer : un afficheur montre un code à saisir sur le site web pour se connecter.
- Contrat avec B : `surUtilisateurIdentifie(int idUser)` et `surAuthEchouee()`.


|Partie	|État|
|--|--|
|Format des messages, analyser(), commandes CODE: / CLEAR / ENROLL:	|Fait, testé|
|Découpage des lignes reçues, callbacks, simulateur|	Fait, testé|
|Port série (ouvrir, lire, envoyer, fermer)	|À faire (4 TODO)|
|Code Arduino (.ino) : capteur d’empreinte, afficheur, dialogue série|	À faire|
|Test avec le vrai matériel|	À faire|
|Brancher dans main.cpp / Authentificateur avec B	|À faire|