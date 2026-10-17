#include "connection.h"

// ===== Paramètres de connexion : remplace-les par les tiens =====
// SOURCE      = nom de la source de données ODBC créée dans Windows
//               (Sources de données ODBC 64 bits > DSN utilisateur)
// UTILISATEUR = utilisateur Oracle (celui créé pendant l'installation / dans SQL Developer)
// MOT_DE_PASSE = son mot de passe
static const char *SOURCE = "Source_Projet2A";
static const char *UTILISATEUR = "projet2a";
static const char *MOT_DE_PASSE = "hacktime";

Connection::Connection() {}

bool Connection::createconnect()
{
    db = QSqlDatabase::addDatabase("QODBC");  // Oracle via ODBC
    db.setDatabaseName(SOURCE);
    db.setUserName(UTILISATEUR);
    db.setPassword(MOT_DE_PASSE);

    if (db.open()) {
        qDebug() << "Connexion BDD réussie";
        return true;
    }
    qDebug() << "Erreur de connexion BDD :" << db.lastError().text();
    return false;
}

QString Connection::lastError() const
{
    return db.lastError().text();
}

void Connection::closeConnection()
{
    db.close();
}
