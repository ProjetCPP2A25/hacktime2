# HackTime — Gestion intelligente de hackathons

Application Qt (C++) du projet 2A : gestion des compétitions, des participants, etc.

## Structure

```
HackTime.pro              projet principal (à ouvrir dans Qt Creator)
authentification/         page de connexion (bibliothèque)
hackathon/                application : interface et modules
  participants/           module Participants
```

## Compilation

1. Ouvrir `HackTime.pro` dans Qt Creator (Qt 6, kit MinGW 64-bit).
2. *Compiler* → *Recompiler tout*, puis exécuter **hackathon**.

## Comptes de test

| Rôle | Utilisateur | Mot de passe |
|---|---|---|
| Administrateur | hacktime123 | hacktime123 |
| Organisateur | hacktime | hacktime |
| Jury | hacktime321 | hacktime321 |
