#include "crudcompetition.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

static QString lastError;

static bool executer(QSqlQuery &query, const char *action)
{
    if (query.exec()) return true;
    lastError = query.lastError().text();
    qDebug() << action << "impossible :" << lastError;
    return false;
}

// ------------------------------------------------------------- libellés <-> codes
QString Competition::modeVersCode(const QString &mode)
{
    if (mode == "En ligne") return "EN_LIGNE";
    if (mode == "Hybride") return "HYBRIDE";
    return "PRESENTIEL";
}

QString Competition::codeVersMode(const QString &code)
{
    if (code == "EN_LIGNE") return "En ligne";
    if (code == "HYBRIDE") return "Hybride";
    return "Présentiel";
}

QString Competition::statutVersCode(const QString &statut)
{
    if (statut == "En cours") return "EN_COURS";
    if (statut == "Terminée") return "TERMINEE";
    return "PLANIFIEE";
}

QString Competition::codeVersStatut(const QString &code)
{
    if (code == "EN_COURS") return "En cours";
    if (code == "TERMINEE") return "Terminée";
    return "Planifiée";
}

QString Competition::derniereErreur() { return lastError; }

// Rôle de l'application (« Administrateur »…) -> ID de l'employé de ce rôle dans EMPLOYE.
// Si aucun employé n'a ce rôle, on prend le premier employé de la table.
int Competition::idEmployeParRole(const QString &role)
{
    QSqlQuery query;
    query.prepare("SELECT MIN(ID_EMPLOYE) FROM EMPLOYE WHERE ROLE = :role");
    query.bindValue(":role", role.toUpper());
    if (query.exec() && query.next() && !query.value(0).isNull())
        return query.value(0).toInt();
    QSqlQuery first("SELECT MIN(ID_EMPLOYE) FROM EMPLOYE");
    if (first.next()) return first.value(0).toInt();
    return 0;
}

// ------------------------------------------------------------- constructeurs
Competition::Competition() {}

Competition::Competition(const QString &nom, const QString &theme, const QDate &dateDebut,
                         const QDate &dateFin, int nbMaxMembres, const QString &statut,
                         const QString &mode, int idEmploye)
    : nom(nom), theme(theme), dateDebut(dateDebut), dateFin(dateFin), nbMaxMembres(nbMaxMembres),
      statut(statut), mode(mode), idEmploye(idEmploye)
{
}

// ------------------------------------------------------------- contrôle de saisie
// Mêmes règles que les contraintes de la table COMPETITION.
QString Competition::verifier() const
{
    if (nom.trimmed().isEmpty()) return "Le nom de la compétition est obligatoire.";
    if (nom.length() > 100) return "Le nom ne doit pas dépasser 100 caractères.";
    if (theme.trimmed().isEmpty()) return "Le thème est obligatoire.";
    if (theme.length() > 40) return "Le thème ne doit pas dépasser 40 caractères.";
    if (!dateDebut.isValid() || !dateFin.isValid()) return "Les dates sont obligatoires.";
    if (dateFin < dateDebut) return "La date de fin doit être après la date de début.";
    if (nbMaxMembres < 1 || nbMaxMembres > 50) return "Le nombre maximum doit être entre 1 et 50.";
    if (idEmploye <= 0) return "Aucun employé responsable : la table EMPLOYE est vide.";
    return QString();
}

// ------------------------------------------------------------- CRUD
bool Competition::ajouter()
{
    // ID_COMPETITION n'est pas donné : la séquence et le trigger de la table le remplissent.
    QSqlQuery query;
    query.prepare("INSERT INTO COMPETITION (NOM, THEME, DATE_DEBUT, DATE_FIN, NB_MAX_MEMBRES, "
                  "STATUT, MODE_COMP, ID_EMPLOYE) "
                  "VALUES (:nom, :theme, :debut, :fin, :max, :statut, :mode, :employe)");
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":theme", theme);
    query.bindValue(":debut", dateDebut);
    query.bindValue(":fin", dateFin);
    query.bindValue(":max", nbMaxMembres);
    query.bindValue(":statut", statutVersCode(statut));
    query.bindValue(":mode", modeVersCode(mode));
    query.bindValue(":employe", idEmploye);
    if (!executer(query, "Ajout")) return false;

    // ID que le trigger vient de donner
    QSqlQuery last("SELECT MAX(ID_COMPETITION) FROM COMPETITION");
    if (last.next()) id = last.value(0).toInt();
    return true;
}

QSqlQueryModel *Competition::afficher()
{
    auto *model = new QSqlQueryModel();
    model->setQuery("SELECT ID_COMPETITION, NOM, THEME, DATE_DEBUT, DATE_FIN, NB_MAX_MEMBRES, "
                    "STATUT, MODE_COMP, ID_EMPLOYE FROM COMPETITION ORDER BY ID_COMPETITION");
    const QStringList titres = {"ID", "Nom", "Thème", "Début", "Fin", "Membres max",
                                "Statut", "Mode", "Employé"};
    for (int i = 0; i < titres.size(); ++i)
        model->setHeaderData(i, Qt::Horizontal, titres.at(i));
    return model;
}

bool Competition::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE COMPETITION SET NOM = :nom, THEME = :theme, DATE_DEBUT = :debut, "
                  "DATE_FIN = :fin, NB_MAX_MEMBRES = :max, STATUT = :statut, MODE_COMP = :mode, "
                  "ID_EMPLOYE = :employe WHERE ID_COMPETITION = :id");
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":theme", theme);
    query.bindValue(":debut", dateDebut);
    query.bindValue(":fin", dateFin);
    query.bindValue(":max", nbMaxMembres);
    query.bindValue(":statut", statutVersCode(statut));
    query.bindValue(":mode", modeVersCode(mode));
    query.bindValue(":employe", idEmploye);
    query.bindValue(":id", id);
    return executer(query, "Modification");
}

bool Competition::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM COMPETITION WHERE ID_COMPETITION = :id");
    query.bindValue(":id", id);
    return executer(query, "Suppression");
}

bool Competition::existe(int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM COMPETITION WHERE ID_COMPETITION = :id");
    query.bindValue(":id", id);
    return query.exec() && query.next() && query.value(0).toInt() > 0;
}

QVector<Competition> Competition::lister()
{
    QVector<Competition> out;
    QSqlQuery query;
    query.prepare("SELECT ID_COMPETITION, NOM, THEME, DATE_DEBUT, DATE_FIN, NB_MAX_MEMBRES, "
                  "STATUT, MODE_COMP, ID_EMPLOYE FROM COMPETITION ORDER BY ID_COMPETITION");
    if (!executer(query, "Lecture")) return out;
    while (query.next()) {
        Competition c;
        c.id = query.value(0).toInt();
        c.nom = query.value(1).toString();
        c.theme = query.value(2).toString();
        c.dateDebut = query.value(3).toDate();
        c.dateFin = query.value(4).toDate();
        c.nbMaxMembres = query.value(5).toInt();
        c.statut = codeVersStatut(query.value(6).toString());
        c.mode = codeVersMode(query.value(7).toString());
        c.idEmploye = query.value(8).toInt();
        out.append(c);
    }
    return out;
}
