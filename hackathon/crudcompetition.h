// Classe Competition : l'entité gérée par le module « Gestion des compétitions ».
// Elle correspond à la table COMPETITION du modèle relationnel validé :
//   Competition (id_competition, nom, theme, date_debut, date_fin,
//                nb_max_membres, statut, mode, #id_employe)
// Dans Oracle, « mode » est un mot réservé : la colonne s'appelle MODE_COMP.
// CRUD : ajouter / afficher / modifier / supprimer.
//
// Le mode et le statut s'affichent avec accents dans l'application (« Présentiel »,
// « Planifiée ») et sont enregistrés en codes sans accents (PRESENTIEL, PLANIFIEE),
// comme l'imposent les contraintes CHECK de la table.
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
                const QDate &dateFin, int nbMaxMembres, const QString &statut,
                const QString &mode, int idEmploye);

    // getters
    int getId() const { return id; }
    QString getNom() const { return nom; }
    QString getTheme() const { return theme; }
    QDate getDateDebut() const { return dateDebut; }
    QDate getDateFin() const { return dateFin; }
    int getNbMaxMembres() const { return nbMaxMembres; }
    QString getStatut() const { return statut; }
    QString getMode() const { return mode; }
    int getIdEmploye() const { return idEmploye; }

    // setters
    void setId(int v) { id = v; }
    void setNom(const QString &v) { nom = v; }
    void setTheme(const QString &v) { theme = v; }
    void setDateDebut(const QDate &v) { dateDebut = v; }
    void setDateFin(const QDate &v) { dateFin = v; }
    void setNbMaxMembres(int v) { nbMaxMembres = v; }
    void setStatut(const QString &v) { statut = v; }
    void setMode(const QString &v) { mode = v; }
    void setIdEmploye(int v) { idEmploye = v; }

    // contrôle de saisie : message d'erreur, ou chaîne vide si tout est valide
    QString verifier() const;

    // CRUD
    bool ajouter();                          // INSERT ; l'ID créé est ensuite dans getId()
    QSqlQueryModel *afficher();              // SELECT, pour un QTableView
    bool modifier();                         // UPDATE selon l'ID
    static bool supprimer(int id);           // DELETE
    static bool existe(int id);
    static QVector<Competition> lister();    // toutes les compétitions (pour l'interface)
    static QString derniereErreur();         // message Oracle de la dernière erreur

    // employé qui gère la compétition (#id_employe) : celui du rôle connecté
    static int idEmployeParRole(const QString &role);

    // libellé affiché <-> code enregistré dans la base
    static QString modeVersCode(const QString &mode);
    static QString codeVersMode(const QString &code);
    static QString statutVersCode(const QString &statut);
    static QString codeVersStatut(const QString &code);

private:
    int id = 0;
    QString nom, theme;
    QDate dateDebut, dateFin;
    int nbMaxMembres = 0;
    QString statut, mode;   // libellés affichés
    int idEmploye = 0;      // clé étrangère vers EMPLOYE (obligatoire)
};

#endif // CRUDCOMPETITION_H
