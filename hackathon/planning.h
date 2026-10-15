// Métier innovant du module Compétitions : détection automatique des conflits de planning.
// Avant de valider une compétition, on vérifie :
//   - les dates   : cohérence début/fin, durée, statut par rapport à la date du jour ;
//   - les salles  : une salle ne peut pas accueillir deux compétitions aux mêmes dates
//                   (une compétition « En ligne » n'occupe aucune salle) ;
//   - les jurys   : un membre du jury ne peut pas juger deux compétitions aux mêmes dates.
// Les conflits de salle et de jury sont bloquants ; les remarques sur les dates sont
// de simples avertissements (on peut enregistrer quand même).
#ifndef PLANNING_H
#define PLANNING_H

#include <QDate>
#include <QSet>
#include <QStringList>
#include <QVector>
#include "competition.h"

struct Conflict {
    enum Kind { Salle, Jury, Date };
    Kind kind;
    bool blocking;     // true -> la compétition ne peut pas être validée
    QString otherId;   // compétition en conflit (vide pour une remarque sur les dates)
    QString message;
};

// « Dr. Ben Ali, Mme Trabelsi » -> {"Dr. Ben Ali", "Mme Trabelsi"}
inline QStringList juryMembers(const QString &jury) {
    QStringList out;
    const QStringList parts = jury.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString m = part.simplified();
        if (!m.isEmpty()) out << m;
    }
    return out;
}

// Deux compétitions se chevauchent si leurs périodes [début, fin] ont au moins un jour commun.
inline bool overlaps(const Comp &a, const Comp &b) {
    const QDate d1 = QDate::fromString(a.debut, DATE_FMT), f1 = QDate::fromString(a.fin, DATE_FMT);
    const QDate d2 = QDate::fromString(b.debut, DATE_FMT), f2 = QDate::fromString(b.fin, DATE_FMT);
    if (!d1.isValid() || !f1.isValid() || !d2.isValid() || !f2.isValid()) return false;
    return d1 <= f2 && d2 <= f1;
}

// Tous les conflits de la compétition « c » face aux compétitions « all »
// (c elle-même, reconnue par son ID, est ignorée).
inline QVector<Conflict> detectConflicts(const Comp &c, const QVector<Comp> &all) {
    QVector<Conflict> out;
    const QDate debut = QDate::fromString(c.debut, DATE_FMT), fin = QDate::fromString(c.fin, DATE_FMT);
    const QDate today = QDate::currentDate();

    // --- 1. dates
    if (!debut.isValid() || !fin.isValid()) {
        out << Conflict{Conflict::Date, true, QString(), "Les dates de la compétition sont invalides."};
        return out;
    }
    if (fin < debut)
        out << Conflict{Conflict::Date, true, QString(), "La date de fin est avant la date de début."};
    const qint64 days = debut.daysTo(fin) + 1;
    if (days > 7)
        out << Conflict{Conflict::Date, false, QString(),
                        QString("Durée de %1 jours : inhabituelle pour un hackathon (7 jours au plus).").arg(days)};
    if (c.statut == "Planifiée" && debut < today)
        out << Conflict{Conflict::Date, false, QString(),
                        "Statut « Planifiée » alors que la date de début est déjà passée."};
    if (c.statut == "En cours" && (today < debut || today > fin))
        out << Conflict{Conflict::Date, false, QString(),
                        "Statut « En cours » alors qu'aujourd'hui n'est pas dans la période de la compétition."};
    if (c.statut == "Terminée" && fin >= today)
        out << Conflict{Conflict::Date, false, QString(),
                        "Statut « Terminée » alors que la compétition n'est pas encore finie."};

    // --- 2. salles et 3. jurys, contre chaque compétition qui chevauche
    const bool online = c.mode == "En ligne";
    const QString salle = online ? QString() : c.salle.simplified();
    if (!online && salle.isEmpty())
        out << Conflict{Conflict::Salle, false, QString(),
                        QString("Aucune salle choisie pour une compétition « %1 ».").arg(c.mode)};
    QSet<QString> jury;
    for (const QString &m : juryMembers(c.jury)) jury.insert(m.toLower());

    for (const Comp &o : all) {
        if (o.id == c.id || !overlaps(c, o)) continue;
        const QString who = QString("%1 « %2 » (du %3 au %4)").arg(o.id, o.nom, o.debut, o.fin);

        if (!salle.isEmpty() && o.mode != "En ligne"
            && salle.compare(o.salle.simplified(), Qt::CaseInsensitive) == 0)
            out << Conflict{Conflict::Salle, true, o.id,
                            QString("Salle « %1 » déjà réservée par %2.").arg(salle, who)};

        QStringList shared;
        for (const QString &m : juryMembers(o.jury))
            if (jury.contains(m.toLower())) shared << m;
        if (!shared.isEmpty())
            out << Conflict{Conflict::Jury, true, o.id,
                            QString("Jury déjà occupé par %1 : %2.").arg(who, shared.join(", "))};
    }
    return out;
}

inline bool hasBlocking(const QVector<Conflict> &conflicts) {
    for (const Conflict &k : conflicts)
        if (k.blocking) return true;
    return false;
}

// Suggestion : premier jour de début, à partir du lendemain de la date demandée, pour lequel
// la même durée ne provoque plus aucun conflit de salle ni de jury. Date invalide si rien
// n'est libre dans l'année qui suit.
inline QDate nextFreeStart(const Comp &c, const QVector<Comp> &all) {
    const QDate debut = QDate::fromString(c.debut, DATE_FMT), fin = QDate::fromString(c.fin, DATE_FMT);
    if (!debut.isValid() || !fin.isValid() || fin < debut) return QDate();
    const qint64 length = debut.daysTo(fin);
    for (int shift = 1; shift <= 365; ++shift) {
        Comp candidate = c;
        candidate.debut = debut.addDays(shift).toString(DATE_FMT);
        candidate.fin = debut.addDays(shift + length).toString(DATE_FMT);
        bool free = true;
        for (const Conflict &k : detectConflicts(candidate, all))
            if (k.blocking && k.kind != Conflict::Date) { free = false; break; }
        if (free) return debut.addDays(shift);
    }
    return QDate();
}

#endif // PLANNING_H
