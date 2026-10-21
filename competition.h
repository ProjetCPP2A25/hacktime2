// Module Gestion des compétitions : la classe Competition + les listes de choix et les couleurs de statut.
#ifndef COMPETITION_H
#define COMPETITION_H

#include <QSqlQueryModel>
#include <QString>
#include <QStringList>
#include <QVector>

// =====================================================================
//  Classe Competition : l'entité gérée par le module « Gestion des compétitions »
//  (une seule classe : attributs + CRUD sur la table COMPETITION d'Oracle).
//
//  Table COMPETITION (modèle relationnel validé) :
//    Competition (id_competition, nom, theme, date_debut, date_fin,
//                 nb_max_membres, statut, mode)
//    Gerer       (#id_employe, #id_competition)   <- association n,n Employe / Competition
//  « mode » est un mot réservé d'Oracle : la colonne s'appelle MODE_COMP.
//
//  Salle, jury, description et sponsors sont affichés dans l'interface mais ne
//  sont pas des colonnes de la table : ils sont gardés à part par l'application.
// =====================================================================
class Competition
{
public:
    Competition();
    Competition(const QString &nom, const QString &theme, const QString &debut,
                const QString &fin, int max, const QString &statut, const QString &mode);

    // ----- getters
    QString getId() const { return id; }
    QString getNom() const { return nom; }
    QString getTheme() const { return theme; }
    QString getDebut() const { return debut; }     // jj/MM/aaaa
    QString getFin() const { return fin; }         // jj/MM/aaaa
    int getMax() const { return max; }             // nb_max_membres
    QString getStatut() const { return statut; }   // « Planifiée », « En cours », « Terminée »
    QString getMode() const { return mode; }       // « Présentiel », « En ligne », « Hybride »
    int getIdEmploye() const { return idEmploye; }
    QString getSalle() const { return salle; }
    QString getJury() const { return jury; }
    QString getDescription() const { return description; }
    QString getSponsors() const { return sponsors; }

    // ----- setters
    void setId(const QString &v) { id = v; }
    void setNom(const QString &v) { nom = v; }
    void setTheme(const QString &v) { theme = v; }
    void setDebut(const QString &v) { debut = v; }
    void setFin(const QString &v) { fin = v; }
    void setMax(int v) { max = v; }
    void setStatut(const QString &v) { statut = v; }
    void setMode(const QString &v) { mode = v; }
    void setIdEmploye(int v) { idEmploye = v; }
    void setSalle(const QString &v) { salle = v; }
    void setJury(const QString &v) { jury = v; }
    void setDescription(const QString &v) { description = v; }
    void setSponsors(const QString &v) { sponsors = v; }

    // ----- contrôle de saisie : message d'erreur, ou chaîne vide si tout est valide
    QString verifier() const;

    // ----- CRUD
    bool ajouter();                          // INSERT dans COMPETITION + GERER ; l'ID donné par Oracle est ensuite dans getId()
    QSqlQueryModel *afficher();              // SELECT, pour un QTableView
    bool modifier();                         // UPDATE selon l'ID
    static bool supprimer(int id);           // DELETE
    static bool existe(int id);
    static QVector<Competition> lister();    // toutes les compétitions de la table
    static QString derniereErreur();         // message Oracle de la dernière erreur

    // employé du rôle connecté (« Administrateur »…) : responsable des nouvelles compétitions
    static int idEmployeParRole(const QString &role);

    // libellé affiché <-> code enregistré dans la base
    static QString modeVersCode(const QString &mode);
    static QString codeVersMode(const QString &code);
    static QString statutVersCode(const QString &statut);
    static QString codeVersStatut(const QString &code);

private:
    // colonnes de la table COMPETITION
    QString id;                 // ID_COMPETITION (« Auto » tant qu'elle n'est pas enregistrée)
    QString nom, theme;
    QString debut, fin;         // DATE_DEBUT, DATE_FIN
    int max = 5;                // NB_MAX_MEMBRES
    QString statut, mode;       // STATUT, MODE_COMP
    int idEmploye = 0;          // employe qui gere la competition (ligne de GERER)
    // informations de l'interface (hors table)
    QString salle, jury, description, sponsors;
};

static const QString DATE_FMT = "dd/MM/yyyy";
static const int PER_PAGE = 8;

static const QStringList THEMES = {"Environnement", "Santé", "Éducation", "Ville intelligente",
                                   "Agriculture", "Intelligence artificielle", "Développement",
                                   "Finance", "Autre"};
static const QStringList MODES = {"Présentiel", "En ligne", "Hybride"};
static const QStringList STATUTS = {"Planifiée", "En cours", "Terminée"};
static const QStringList SALLES = {"Amphi 1", "Amphi 2", "Salle A", "Salle B",
                                   "Lab Innovation", "Espace Coworking"};

inline QString themeIcon(const QString &t) {
    if (t == "Environnement") return "🍃";
    if (t == "Santé") return "❤️";
    if (t == "Éducation") return "🎓";
    if (t == "Ville intelligente") return "🏙️";
    if (t == "Agriculture") return "🌱";
    if (t == "Intelligence artificielle") return "🤖";
    if (t == "Développement") return "</>";
    if (t == "Finance") return "🪙";
    return "⭐";
}
// « A, B , C » -> {"A", "B", "C"} (jury, sponsors…)
inline QStringList splitNames(const QString &text) {
    QStringList out;
    const QStringList parts = text.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString n = part.simplified();
        if (!n.isEmpty()) out << n;
    }
    return out;
}

inline QString modeIcon(const QString &m) {
    if (m == "En ligne") return "💻";
    if (m == "Hybride") return "🔀";
    return "🏢";
}
inline QString statutBg(const QString &s) {
    if (s == "En cours") return "#DDF3E4";
    if (s == "Terminée") return "#E5E9F0";
    return "#DCE9FF";
}
inline QString statutFg(const QString &s) {
    if (s == "En cours") return "#2E9E5B";
    if (s == "Terminée") return "#64748B";
    return "#2563EB";
}
inline QString statutDot(const QString &s) {
    if (s == "En cours") return "#2E9E5B";
    if (s == "Terminée") return "#94A3B8";
    return "#2563EB";
}

#endif // COMPETITION_H
