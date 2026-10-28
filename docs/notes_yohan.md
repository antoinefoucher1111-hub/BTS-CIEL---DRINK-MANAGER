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