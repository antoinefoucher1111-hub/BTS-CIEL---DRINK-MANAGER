# BTS-CIEL---DRINK-MANAGER
BTS CIEL - DRINK MANAGER

## Projet POO 


## Diagramme Relation d'identité POO Drink Manager :

<img width="2734" height="2268" alt="Diagramme Entité-Relation (DER)" src="https://github.com/user-attachments/assets/874e1c1a-1fbe-4e89-9c22-78a027e3a250" />

## Diagramme d'architecture

<img width="2293" height="907" alt="DiagramProjectLink" src="https://github.com/user-attachments/assets/026578a6-e3c8-4c66-a143-7070e90bf871" />


## Arborescence du projet
```
DrinkManager/
├── .gitignore
├── README.md
├── CMakeLists.txt      
├── src/
│   ├── main.cpp
│   ├── materiel/
│   │   ├── protocole.h / .cpp
│   │   └── lecteurempreinte.h / .cpp
│   ├── securite/     
│   │   ├── securite.h / .cpp
│   │   ├── qrgenerateur.h / .cpp
│   │   └── portefeuille.h / .cpp
│   ├── bdd/ 
│   ├── ui/   
│   └── authentificateur.h / .cpp
├── tests/
│   ├── test_protocole.cpp 
│   └── test_portefeuille.cpp 
├── arduino/
│   └── lecteur_empreinte/
│       └── lecteur_empreinte.ino   
├── third_party/
│   └── qrcodegen/          
└── docs/
    ├── contrat.md          
    ├── notes_yohan.md   
    └── notes_binome.md
```
