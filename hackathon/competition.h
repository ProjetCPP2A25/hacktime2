// Modèle de données : une compétition + les listes de choix et les couleurs de statut.
#ifndef COMPETITION_H
#define COMPETITION_H

#include <QString>
#include <QStringList>

struct Comp {
    QString id, nom, theme, debut, fin, mode, statut;
    int max = 5;
    QString salle;   // ex. « Amphi 1 »
    QString jury;    // membres du jury séparés par des virgules
    QString description;
    QString sponsors;  // sponsors séparés par des virgules
    int idEmploye = 0; // employé responsable (#id_employe dans la base)
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
