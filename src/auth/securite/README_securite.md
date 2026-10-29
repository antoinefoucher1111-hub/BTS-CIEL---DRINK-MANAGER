# Package Sécurité

But du package est de faire la sécurité entre le rasp pi et l'arduino pour communiquer le lecteur d'empreinte.

Ce package tourne sur le Pi.

L'arduino donne un ID (finger print id) -> appelle surUtilisateurIdentifie à l'authentificateur.
reste à faire : package matériel :

auth test => lien arduino

TO DO :

- [ ] Vérifier unicité du code généré.
- [ ] Vérifier la similarité du code généré et entré dans l'application web

## Principe de fonctionnement

```mermaid
flowchart LR
C[CapteursFonction :<br> Transmet un id à l'Arduino]-->A(Arduino <br>Fonctions : <br> 1. Transmet l'id au Raspberry <br> 2. Affiche le code Auth)
A-->Rasp[Fonctions : <br> 1. Récupère l'id et génère le jeton associé <br> 2. Authentifie l'utilisateur via l'appli web <br>3. Renvoie le code généré aléatoirement à l'Arduino]
Rasp-->A
A-->E[Ecran]
```

L'id doit référer à un utilisateur en base, lié par un jeton temporaire : jeton = fingerId + timestamp + code unique.

Le raspberry fait les calculs de hashage.
