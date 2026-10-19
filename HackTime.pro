# Projet HackTime : deux sous-projets reliés.
#   authentification : la page de connexion (bibliothèque)
#   hackathon        : l'interface du projet (application), qui utilise authentification
TEMPLATE = subdirs

SUBDIRS = \
    authentification \
    hackathon

hackathon.depends = authentification
