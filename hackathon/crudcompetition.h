// Classe Competition : l'entité gérée par le module « Gestion des compétitions ».
// Elle correspond à la table COMPETITION de la base Oracle commune :
//   ID_COMPETITION (clé primaire, remplie par la séquence + trigger), NOM, THEME,
//   DATE_DEBUT, DATE_FIN, MODE_COMP, MAX_EQUIPES, STATUT, DESCRIPTION,
//   #ID_SALLE (-> SALLE), #ID_ORGANISATEUR (-> EMPLOYE)
// CRUD : ajouter / afficher / modifier / supprimer.
//
// Dans l'application, le mode et le statut s'affichent avec accents (« Présentiel »,
// « Planifiée »). Dans la base, ils sont enregistrés en codes sans accents
// (PRESENTIEL, PLANIFIEE), comme l'imposent les contraintes CHECK de la table.
#ifndef CRUDCOMPETITION_H
#define CRUDCOMPETITION_H

#include <QDate>
#include <QSqlQueryModel>
#include <QString>
#include <QVector>

class Competition
{
public:
    Competition();
    Competition(const QString &nom, const QString &theme, const QDate &dateDebut,
                const QDate &dateFin, const QString &mode, int maxEquipes, const QString &statut,
                const QString &description, int idSalle = 0, int idOrganisateur = 0);

    // getters
    int getId() const { return id; }
    QString getNom() const { return nom; }
    QString getTheme() const { return theme; }
    QDate getDateDebut() const { return dateDebut; }
    QDate getDateFin() const { return dateFin; }
    QString getMode() const { return mode; }
    int getMaxEquipes() const { return maxEquipes; }
    QString getStatut() const { return statut; }
    QString getDescription() const { return description; }
    int getIdSalle() const { return idSalle; }
    int getIdOrganisateur() const { return idOrganisateur; }

    // setters
    void setId(int v) { id = v; }
    void setNom(const QString &v) { nom = v; }
    void setTheme(const QString &v) { theme = v; }
    void setDateDebut(const QDate &v) { dateDebut = v; }
    void setDateFin(const QDate &v) { dateFin = v; }
    void setMode(const QString &v) { mode = v; }
    void setMaxEquipes(int v) { maxEquipes = v; }
    void setStatut(const QString &v) { statut = v; }
    void setDescription(const QString &v) { description = v; }
    void setIdSalle(int v) { idSalle = v; }
    void setIdOrganisateur(int v) { idOrganisateur = v; }

    // contrôle de saisie : renvoie un message d'erreur, ou une chaîne vide si tout est valide
    QString verifier() const;

    // CRUD
    bool ajouter();                          // INSERT ; l'ID créé est ensuite dans getId()
    QSqlQueryModel *afficher();              // SELECT, pour un QTableView
    bool modifier();                         // UPDATE selon l'ID
    static bool supprimer(int id);           // DELETE
    static bool existe(int id);
    static QVector<Competition> lister();    // toutes les compétitions (pour l'interface)
    static QString derniereErreur();         // message Oracle de la dernière erreur

    // libellé affiché <-> code enregistré dans la base
    static QString modeVersCode(const QString &mode);
    static QString codeVersMode(const QString &code);
    static QString statutVersCode(const QString &statut);
    static QString codeVersStatut(const QString &code);

private:
    bool insert(bool avecId);   // INSERT, avec ou sans ID explicite

    int id = 0;
    QString nom, theme;
    QDate dateDebut, dateFin;
    QString mode, statut, description;   // mode et statut : libellés affichés
    int maxEquipes = 0;
    int idSalle = 0;          // 0 = aucune salle (NULL dans la base)
    int idOrganisateur = 0;   // 0 = aucun organisateur (NULL dans la base)
};

#endif // CRUDCOMPETITION_H
