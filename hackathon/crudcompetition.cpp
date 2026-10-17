#include "crudcompetition.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

static QString lastError;

// une clé étrangère à 0 est enregistrée comme NULL
static QVariant cleOuNull(int v)
{
    return v > 0 ? QVariant(v) : QVariant(QMetaType::fromType<int>());
}

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

// ------------------------------------------------------------- constructeurs
Competition::Competition() {}

Competition::Competition(const QString &nom, const QString &theme, const QDate &dateDebut,
                         const QDate &dateFin, const QString &mode, int maxEquipes,
                         const QString &statut, const QString &description, int idSalle,
                         int idOrganisateur)
    : nom(nom), theme(theme), dateDebut(dateDebut), dateFin(dateFin), mode(mode),
      statut(statut), description(description), maxEquipes(maxEquipes), idSalle(idSalle),
      idOrganisateur(idOrganisateur)
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
    if (maxEquipes < 1 || maxEquipes > 50) return "Le nombre d'équipes doit être entre 1 et 50.";
    if (description.length() > 500) return "La description ne doit pas dépasser 500 caractères.";
    return QString();
}

// ------------------------------------------------------------- CRUD
bool Competition::ajouter()
{
    // ID_COMPETITION n'est pas donné : la séquence et le trigger de la table le remplissent.
    if (insert(false)) return true;
    // pas de trigger sur la table : on prend le plus grand ID + 1
    return insert(true);
}

bool Competition::insert(bool avecId)
{
    int nouvelId = 0;
    if (avecId) {
        QSqlQuery maxQuery("SELECT NVL(MAX(ID_COMPETITION), 0) + 1 FROM COMPETITION");
        if (!maxQuery.next()) return false;
        nouvelId = maxQuery.value(0).toInt();
    }
    QSqlQuery query;
    query.prepare(QString("INSERT INTO COMPETITION (%1NOM, THEME, DATE_DEBUT, DATE_FIN, MODE_COMP, "
                          "MAX_EQUIPES, STATUT, DESCRIPTION, ID_SALLE, ID_ORGANISATEUR) "
                          "VALUES (%2:nom, :theme, :debut, :fin, :mode, :max, :statut, :description, "
                          ":salle, :orga)")
                      .arg(avecId ? QString("ID_COMPETITION, ") : QString(),
                           avecId ? QString(":id, ") : QString()));
    if (avecId) query.bindValue(":id", nouvelId);
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":theme", theme);
    query.bindValue(":debut", dateDebut);
    query.bindValue(":fin", dateFin);
    query.bindValue(":mode", modeVersCode(mode));
    query.bindValue(":max", maxEquipes);
    query.bindValue(":statut", statutVersCode(statut));
    query.bindValue(":description", description);
    query.bindValue(":salle", cleOuNull(idSalle));
    query.bindValue(":orga", cleOuNull(idOrganisateur));
    if (!executer(query, "Ajout")) return false;

    if (avecId) {
        id = nouvelId;
    } else {  // récupérer l'ID que le trigger vient de donner
        QSqlQuery last("SELECT MAX(ID_COMPETITION) FROM COMPETITION");
        if (last.next()) id = last.value(0).toInt();
    }
    return true;
}

QSqlQueryModel *Competition::afficher()
{
    auto *model = new QSqlQueryModel();
    model->setQuery("SELECT ID_COMPETITION, NOM, THEME, DATE_DEBUT, DATE_FIN, MODE_COMP, "
                    "MAX_EQUIPES, STATUT, DESCRIPTION, ID_SALLE, ID_ORGANISATEUR "
                    "FROM COMPETITION ORDER BY ID_COMPETITION");
    const QStringList titres = {"ID", "Nom", "Thème", "Début", "Fin", "Mode",
                                "Équipes max", "Statut", "Description", "Salle", "Organisateur"};
    for (int i = 0; i < titres.size(); ++i)
        model->setHeaderData(i, Qt::Horizontal, titres.at(i));
    return model;
}

bool Competition::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE COMPETITION SET NOM = :nom, THEME = :theme, DATE_DEBUT = :debut, "
                  "DATE_FIN = :fin, MODE_COMP = :mode, MAX_EQUIPES = :max, STATUT = :statut, "
                  "DESCRIPTION = :description, ID_SALLE = :salle, ID_ORGANISATEUR = :orga "
                  "WHERE ID_COMPETITION = :id");
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":theme", theme);
    query.bindValue(":debut", dateDebut);
    query.bindValue(":fin", dateFin);
    query.bindValue(":mode", modeVersCode(mode));
    query.bindValue(":max", maxEquipes);
    query.bindValue(":statut", statutVersCode(statut));
    query.bindValue(":description", description);
    query.bindValue(":salle", cleOuNull(idSalle));
    query.bindValue(":orga", cleOuNull(idOrganisateur));
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
    query.prepare("SELECT ID_COMPETITION, NOM, THEME, DATE_DEBUT, DATE_FIN, MODE_COMP, "
                  "MAX_EQUIPES, STATUT, DESCRIPTION, ID_SALLE, ID_ORGANISATEUR "
                  "FROM COMPETITION ORDER BY ID_COMPETITION");
    if (!executer(query, "Lecture")) return out;
    while (query.next()) {
        Competition c;
        c.id = query.value(0).toInt();
        c.nom = query.value(1).toString();
        c.theme = query.value(2).toString();
        c.dateDebut = query.value(3).toDate();
        c.dateFin = query.value(4).toDate();
        c.mode = codeVersMode(query.value(5).toString());
        c.maxEquipes = query.value(6).toInt();
        c.statut = codeVersStatut(query.value(7).toString());
        c.description = query.value(8).toString();
        c.idSalle = query.value(9).toInt();          // NULL -> 0
        c.idOrganisateur = query.value(10).toInt();  // NULL -> 0
        out.append(c);
    }
    return out;
}
