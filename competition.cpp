#include "competition.h"

#include <QDate>
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

// ------------------------------------------------------------- constructeurs
Competition::Competition() {}

Competition::Competition(const QString &nom, const QString &theme, const QString &debut,
                         const QString &fin, int max, const QString &statut, const QString &mode)
    : nom(nom), theme(theme), debut(debut), fin(fin), max(max), statut(statut), mode(mode)
{
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

// Rôle de l'application -> ID de l'employé de ce rôle dans EMPLOYE
// (à défaut, le premier employé de la table).
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

// ------------------------------------------------------------- contrôle de saisie
// Mêmes règles que les contraintes de la table COMPETITION.
QString Competition::verifier() const
{
    const QDate d = QDate::fromString(debut, DATE_FMT), f = QDate::fromString(fin, DATE_FMT);
    if (nom.trimmed().isEmpty()) return "Le nom de la compétition est obligatoire.";
    if (nom.length() > 100) return "Le nom ne doit pas dépasser 100 caractères.";
    if (theme.trimmed().isEmpty()) return "Le thème est obligatoire.";
    if (theme.length() > 40) return "Le thème ne doit pas dépasser 40 caractères.";
    if (!d.isValid() || !f.isValid()) return "Les dates sont obligatoires.";
    if (f < d) return "La date de fin doit être après la date de début.";
    if (max < 1 || max > 50) return "Le nombre maximum doit être entre 1 et 50.";
    if (idEmploye <= 0) return "Aucun employé responsable : la table EMPLOYE est vide.";
    return QString();
}

// ------------------------------------------------------------- CRUD
bool Competition::ajouter()
{
    // ID_COMPETITION n'est pas donné : la séquence et le trigger de la table le remplissent.
    QSqlQuery query;
    query.prepare("INSERT INTO COMPETITION (NOM, THEME, DATE_DEBUT, DATE_FIN, NB_MAX_MEMBRES, "
                  "STATUT, MODE_COMP) "
                  "VALUES (:nom, :theme, :debut, :fin, :max, :statut, :mode)");
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":theme", theme);
    query.bindValue(":debut", QDate::fromString(debut, DATE_FMT));
    query.bindValue(":fin", QDate::fromString(fin, DATE_FMT));
    query.bindValue(":max", max);
    query.bindValue(":statut", statutVersCode(statut));
    query.bindValue(":mode", modeVersCode(mode));
    if (!executer(query, "Ajout")) return false;

    // ID que le trigger vient de donner
    QSqlQuery last("SELECT MAX(ID_COMPETITION) FROM COMPETITION");
    if (last.next()) id = QString::number(last.value(0).toInt());

    // association « gerer » (n,n) : l'employe connecte gere cette competition
    QSqlQuery gerer;
    gerer.prepare("INSERT INTO GERER (ID_EMPLOYE, ID_COMPETITION) VALUES (:employe, :id)");
    gerer.bindValue(":employe", idEmploye);
    gerer.bindValue(":id", id.toInt());
    return executer(gerer, "Ajout dans GERER");
}

QSqlQueryModel *Competition::afficher()
{
    auto *model = new QSqlQueryModel();
    model->setQuery("SELECT ID_COMPETITION, NOM, THEME, DATE_DEBUT, DATE_FIN, NB_MAX_MEMBRES, "
                    "STATUT, MODE_COMP FROM COMPETITION ORDER BY ID_COMPETITION");
    const QStringList titres = {"ID", "Nom", "Thème", "Début", "Fin", "Membres max",
                                "Statut", "Mode"};
    for (int i = 0; i < titres.size(); ++i)
        model->setHeaderData(i, Qt::Horizontal, titres.at(i));
    return model;
}

bool Competition::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE COMPETITION SET NOM = :nom, THEME = :theme, DATE_DEBUT = :debut, "
                  "DATE_FIN = :fin, NB_MAX_MEMBRES = :max, STATUT = :statut, MODE_COMP = :mode "
                  "WHERE ID_COMPETITION = :id");
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":theme", theme);
    query.bindValue(":debut", QDate::fromString(debut, DATE_FMT));
    query.bindValue(":fin", QDate::fromString(fin, DATE_FMT));
    query.bindValue(":max", max);
    query.bindValue(":statut", statutVersCode(statut));
    query.bindValue(":mode", modeVersCode(mode));
    query.bindValue(":id", id.toInt());
    return executer(query, "Modification");
}

// Les lignes de GERER, ATELIER et EQUIPE liees sont supprimees par ON DELETE CASCADE.
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
    query.prepare("SELECT c.ID_COMPETITION, c.NOM, c.THEME, c.DATE_DEBUT, c.DATE_FIN, c.NB_MAX_MEMBRES, "
                  "c.STATUT, c.MODE_COMP, "
                  "(SELECT MIN(g.ID_EMPLOYE) FROM GERER g WHERE g.ID_COMPETITION = c.ID_COMPETITION) "
                  "FROM COMPETITION c ORDER BY c.ID_COMPETITION");
    if (!executer(query, "Lecture")) return out;
    while (query.next()) {
        Competition c;
        c.id = QString::number(query.value(0).toInt());
        c.nom = query.value(1).toString();
        c.theme = query.value(2).toString();
        c.debut = query.value(3).toDate().toString(DATE_FMT);
        c.fin = query.value(4).toDate().toString(DATE_FMT);
        c.max = query.value(5).toInt();
        c.statut = codeVersStatut(query.value(6).toString());
        c.mode = codeVersMode(query.value(7).toString());
        c.idEmploye = query.value(8).toInt();   // un des employes qui la gerent (table GERER)
        out.append(c);
    }
    return out;
}
