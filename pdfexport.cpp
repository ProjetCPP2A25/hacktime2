#include "pdfexport.h"

#include <QDate>
#include <QLocale>
#include <QMap>
#include <QSet>
#include <algorithm>

namespace {

const qreal LW = 842, LH = 595, LM = 30;
const QColor NAVY("#0F1B2D"), GREY("#64748B"), DARK("#0E1A2B"), LINE("#E2E8F0");

QFont font(qreal size, bool bold = false) {
    QFont f("Segoe UI");
    f.setPixelSize(qMax(1, qRound(size)));  // à 72 dpi, 1 pixel = 1 point
    f.setBold(bold);
    return f;
}

QDate dateOf(const QString &s) { return QDate::fromString(s, DATE_FMT); }

QString today() { return QLocale(QLocale::French).toString(QDate::currentDate(), "d MMMM yyyy"); }

// « nov. 2026 »
QString monthLabel(const QDate &d) { return QLocale(QLocale::French).toString(d, "MMM yyyy"); }

// Une couleur par thème, la même dans la frise et dans les statistiques.
QColor themeColor(const QString &theme) {
    static const QMap<QString, QString> colors = {
        {"Environnement", "#16A34A"}, {"Santé", "#DC2626"}, {"Éducation", "#7C3AED"},
        {"Ville intelligente", "#0891B2"}, {"Agriculture", "#65A30D"},
        {"Intelligence artificielle", "#2563EB"}, {"Développement", "#EA580C"},
        {"Finance", "#CA8A04"}, {"Autre", "#64748B"}};
    return QColor(colors.value(theme, "#64748B"));
}

// Texte sur une ligne, coupé avec « … » s'il est trop long pour la largeur.
void drawElided(QPainter &p, const QRectF &r, int align, const QString &text) {
    p.drawText(r, align | Qt::AlignVCenter, p.fontMetrics().elidedText(text, Qt::ElideRight, int(r.width())));
}

// ----------------------------------------------------------------- pagination
struct Pager {
    QPdfWriter &writer;
    QPainter &p;
    const QImage &logo;
    int page = 0;

    // Nouvelle page : bandeau sombre avec le logo, titre de la partie, pied de page.
    void start(const QString &part, const QString &title) {
        if (page > 0) writer.newPage();
        ++page;
        p.fillRect(QRectF(0, 0, LW, LH), Qt::white);
        p.fillRect(QRectF(0, 0, LW, 62), DARK);
        p.drawImage(QRectF(LM, 10, 148, 42), logo);
        p.setPen(Qt::white);
        p.setFont(font(18, true));
        p.drawText(QRectF(LM, 0, LW - 2 * LM, 62), Qt::AlignRight | Qt::AlignVCenter, part);
        p.setPen(NAVY);
        p.setFont(font(18, true));
        p.drawText(QRectF(LM, 70, LW - 2 * LM, 30), Qt::AlignLeft | Qt::AlignVCenter, title);
        p.setPen(GREY);
        p.setFont(font(9));
        const QRectF r(LM, LH - 24, LW - 2 * LM, 14);
        p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, "HackTime  ·  Document généré le " + today());
        p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, QString("Page %1").arg(page));
    }
};

// ----------------------------------------------------------------- partie 1 : sponsors

// Frise : une ligne par sponsor, une barre par compétition placée à ses dates, colorée par thème.
void drawSponsorTimeline(Pager &pg, const QStringList &sponsors, const QMap<QString, QVector<Competition>> &bySponsor,
                         const QDate &from, const QDate &to) {
    QPainter &p = pg.p;
    const qreal nameW = 150, x0 = LM + nameW, x1 = LW - LM;
    const qreal top = 112, legendH = 30, bottom = LH - 40 - legendH, axisH = 22;
    const int perPage = qMax(1, int((bottom - top - axisH) / 22));
    const qreal span = qMax<qint64>(1, from.daysTo(to) + 1);
    auto X = [&](const QDate &d) { return x0 + (x1 - x0) * from.daysTo(d) / span; };

    for (int first = 0; first < sponsors.size(); first += perPage) {
        pg.start("Planning des sponsors",
                 first == 0 ? QString("Planning des sponsors") : QString("Planning des sponsors (suite)"));
        const int count = qMin(perPage, int(sponsors.size()) - first);
        const qreal rowH = qMin<qreal>(30, (bottom - top - axisH) / count);
        const qreal gridBottom = top + axisH + count * rowH;

        // mois : bandes alternées + nom du mois
        for (QDate m(from.year(), from.month(), 1); m <= to; m = m.addMonths(1)) {
            const QDate mEnd = qMin(m.addMonths(1), to.addDays(1));
            const qreal a = X(m), b = X(mEnd);
            if (m.month() % 2 == 0) p.fillRect(QRectF(a, top, b - a, gridBottom - top), QColor("#F8FAFC"));
            p.fillRect(QRectF(a, top, 0.6, gridBottom - top), LINE);
            p.setPen(GREY);
            p.setFont(font(9, true));
            drawElided(p, QRectF(a + 4, top, b - a - 6, axisH), Qt::AlignLeft, monthLabel(m));
        }
        p.fillRect(QRectF(LM, top + axisH - 0.8, LW - 2 * LM, 0.8), NAVY);

        QSet<QString> usedThemes;
        for (int i = 0; i < count; ++i) {
            const QString &sponsor = sponsors.at(first + i);
            const qreal y = top + axisH + i * rowH;
            p.fillRect(QRectF(LM, y + rowH - 0.6, LW - 2 * LM, 0.6), LINE);
            p.setPen(NAVY);
            p.setFont(font(10, true));
            drawElided(p, QRectF(LM + 4, y, nameW - 10, rowH), Qt::AlignLeft, sponsor);

            const QVector<Competition> list = bySponsor.value(sponsor);
            for (int k = 0; k < list.size(); ++k) {
                const Competition &c = list.at(k);
                const QDate a = dateOf(c.getDebut()), b = dateOf(c.getFin());
                if (!a.isValid() || !b.isValid()) continue;
                // début de la barre suivante sur la même ligne (pour ne pas écrire l'ID par-dessus)
                const qreal nextX = k + 1 < list.size() && dateOf(list.at(k + 1).getDebut()).isValid()
                                        ? X(dateOf(list.at(k + 1).getDebut())) : x1 + 40;
                usedThemes.insert(c.getTheme());
                const qreal bx = X(a), bw = qMax<qreal>(8, X(b.addDays(1)) - bx);
                const QRectF bar(bx, y + rowH * 0.2, bw, rowH * 0.6);
                p.setPen(Qt::NoPen);
                p.setBrush(themeColor(c.getTheme()));
                p.drawRoundedRect(bar, 3, 3);
                p.setFont(font(7.5, true));
                const qreal tw = p.fontMetrics().horizontalAdvance(c.getId()) + 6;
                if (tw <= bw) {
                    p.setPen(Qt::white);
                    p.drawText(bar, Qt::AlignCenter, c.getId());
                } else if (bar.right() + 3 + tw <= nextX - 2) {
                    p.setPen(themeColor(c.getTheme()));
                    p.drawText(QRectF(bar.right() + 3, bar.top(), tw + 4, bar.height()),
                               Qt::AlignLeft | Qt::AlignVCenter, c.getId());
                }  // sinon pas de place : l'ID se lit dans le tableau qui suit
            }
        }

        // légende des thèmes présents sur la page
        QStringList themes = usedThemes.values();
        std::sort(themes.begin(), themes.end());
        qreal lx = LM;
        const qreal ly = LH - 40 - legendH / 2;
        p.setFont(font(9));
        for (const QString &t : themes) {
            p.setPen(Qt::NoPen);
            p.setBrush(themeColor(t));
            p.drawRoundedRect(QRectF(lx, ly - 5, 14, 10), 2, 2);
            p.setPen(NAVY);
            const qreal w = p.fontMetrics().horizontalAdvance(t);
            p.drawText(QRectF(lx + 19, ly - 8, w + 4, 16), Qt::AlignLeft | Qt::AlignVCenter, t);
            lx += 19 + w + 18;
        }
    }
}

// Tableau détaillé : pour chaque sponsor, ses compétitions avec thème et dates.
void drawSponsorTable(Pager &pg, const QStringList &sponsors, const QMap<QString, QVector<Competition>> &bySponsor) {
    QPainter &p = pg.p;
    struct Col { QString title; qreal width; };
    const QVector<Col> cols = {{"Sponsor", 140}, {"Compétition", 226}, {"Thème", 150},
                               {"Dates", 128}, {"Mode", 64}, {"Statut", 74}};
    const qreal rowH = 22, headH = 24, limit = LH - 40, firstRowY = 108 + 24;
    qreal y = 0;

    auto header = [&](bool suite) {
        pg.start("Planning des sponsors",
                 suite ? QString("Compétitions par sponsor (suite)") : QString("Compétitions par sponsor"));
        y = 108;
        qreal x = LM;
        p.fillRect(QRectF(LM, y, LW - 2 * LM, headH), DARK);
        p.setPen(Qt::white);
        p.setFont(font(9.5, true));
        for (const Col &col : cols) {
            p.drawText(QRectF(x + 8, y, col.width - 12, headH), Qt::AlignLeft | Qt::AlignVCenter, col.title);
            x += col.width;
        }
        y += headH;
    };

    header(false);
    for (int s = 0; s < sponsors.size(); ++s) {
        const QString &sponsor = sponsors.at(s);
        const QVector<Competition> list = bySponsor.value(sponsor);
        for (int i = 0; i < list.size(); ++i) {
            if (y + rowH > limit) header(true);
            const Competition &c = list.at(i);
            const bool firstOfGroup = i == 0, lastOfGroup = i == list.size() - 1;
            if (s % 2 == 1) p.fillRect(QRectF(LM, y, LW - 2 * LM, rowH), QColor("#F8FAFC"));
            p.fillRect(QRectF(LM, y + rowH - (lastOfGroup ? 1.0 : 0.5), LW - 2 * LM, lastOfGroup ? 1.0 : 0.5),
                       lastOfGroup ? QColor("#CBD5E1") : LINE);

            qreal x = LM;
            // le sponsor est écrit sur la première ligne de son groupe, et en haut d'une nouvelle page
            if (firstOfGroup || y < firstRowY + 1) {
                p.setPen(NAVY);
                p.setFont(font(10, true));
                drawElided(p, QRectF(x + 8, y, cols[0].width - 12, rowH), Qt::AlignLeft,
                           firstOfGroup ? sponsor + QString("  (%1)").arg(list.size()) : sponsor);
            }
            x += cols[0].width;
            p.setFont(font(9.5));
            p.setPen(NAVY);
            drawElided(p, QRectF(x + 8, y, cols[1].width - 12, rowH), Qt::AlignLeft, c.getId() + "  " + c.getNom());
            x += cols[1].width;
            p.setPen(Qt::NoPen);
            p.setBrush(themeColor(c.getTheme()));
            p.drawEllipse(QPointF(x + 12, y + rowH / 2), 3.5, 3.5);
            p.setPen(NAVY);
            drawElided(p, QRectF(x + 20, y, cols[2].width - 24, rowH), Qt::AlignLeft, c.getTheme());
            x += cols[2].width;
            drawElided(p, QRectF(x + 8, y, cols[3].width - 12, rowH), Qt::AlignLeft,
                       c.getDebut() == c.getFin() ? c.getDebut() : c.getDebut().left(5) + " → " + c.getFin());
            x += cols[3].width;
            drawElided(p, QRectF(x + 8, y, cols[4].width - 12, rowH), Qt::AlignLeft, c.getMode());
            x += cols[4].width;
            p.setPen(QColor(statutFg(c.getStatut())));
            drawElided(p, QRectF(x + 8, y, cols[5].width - 12, rowH), Qt::AlignLeft, c.getStatut());
            y += rowH;
        }
    }
}

}  // namespace

namespace pdf {

void drawReport(QPdfWriter &writer, QPainter &p, const QVector<Competition> &events, const QImage &logo) {
    Pager pg{writer, p, logo};

    // sponsors -> compétitions, triées par date de début
    QMap<QString, QVector<Competition>> bySponsor;
    QDate from, to;
    for (const Competition &c : events) {
        for (const QString &s : splitNames(c.getSponsors())) bySponsor[s].append(c);
        const QDate a = dateOf(c.getDebut()), b = dateOf(c.getFin());
        if (a.isValid() && (!from.isValid() || a < from)) from = a;
        if (b.isValid() && (!to.isValid() || b > to)) to = b;
    }
    for (auto it = bySponsor.begin(); it != bySponsor.end(); ++it)
        std::sort(it->begin(), it->end(),
                  [](const Competition &a, const Competition &b) { return dateOf(a.getDebut()) < dateOf(b.getDebut()); });
    const QStringList sponsors = bySponsor.keys();  // QMap : déjà dans l'ordre alphabétique

    // --- planning des sponsors
    if (sponsors.isEmpty() || !from.isValid() || !to.isValid()) {
        pg.start("Planning des sponsors", "Planning des sponsors");
        p.setPen(GREY);
        p.setFont(font(12));
        p.drawText(QRectF(LM, 120, LW - 2 * LM, 40), Qt::AlignLeft | Qt::AlignVCenter,
                   "Aucun sponsor n'est renseigné pour les compétitions exportées.");
    } else {
        // la frise va du premier jour du premier mois au dernier jour du dernier mois
        from = QDate(from.year(), from.month(), 1);
        to = QDate(to.year(), to.month(), 1).addMonths(1).addDays(-1);
        drawSponsorTimeline(pg, sponsors, bySponsor, from, to);
        drawSponsorTable(pg, sponsors, bySponsor);
    }
}

}  // namespace pdf
