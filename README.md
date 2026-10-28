# Concept d'application native DRINK MANAGER

## SOMMAIRE :

-

---

## 1. L'application native

Objectifs :

- Lire et écrire dans la base de donnée (type SQLite)
- Créer des users
- Gérer le solde porte-feuille
- Historiser le nombre et l'heure de chaque café pris
- Afficher et interagir avec les données utilisateurs via interface web

On choisit d'écrire le programmme en C++

### Choix du stack technique C++

Langage : C++17 ou C++20 (pour la gestion propre de la mémoire, std::filesystem, et les lambdas). <br>
Base de données : SQLite3 via le wrapper moderne SQLiteCpp (ou les bindings C natifs).<br>
Service Web & API : Crow (micro-framework C++ similaire à Flask, très performant et basé sur Asio) ou CivetWeb.<br>
Fichiers XML : Pugixml (rapide, léger, syntaxe moderne en C++).<br>
Interface Graphique (GUI) locale : Qt ou Raylib / LVGL (idéal pour les écrans tactiles Raspberry Pi en mode kiosk).<br>

---

## 2. L'interface WEB

En C++, le micro-framework [Crow](https://github.com/crowcpp/crow) permet d'exploiter du HTML.

---
