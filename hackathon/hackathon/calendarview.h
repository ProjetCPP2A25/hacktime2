// Calendrier des événements : une vue par mois de toutes les compétitions.
// Chaque compétition est une étiquette sur les jours qu'elle occupe, colorée selon son statut ;
// celles en conflit de planning (salle ou jury) sont entourées de rouge.
// Survol = détails, clic = sélectionne la compétition dans la fenêtre principale.
#ifndef CALENDARVIEW_H
#define CALENDARVIEW_H

#include <QDate>
#include <QDialog>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSet>
#include <QToolTip>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <functional>
#include "competition.h"

// ----------------------------------------------------------------- la grille du mois
class MonthView : public QWidget {
public:
    std::function<void(const QString &)> onPick;  // appelé avec l'ID de la compétition cliquée

    explicit MonthView(QWidget *parent = nullptr) : QWidget(parent) {
        setMouseTracking(true);
        setMinimumSize(760, 520);
    }

    void setEvents(const QVector<Comp> &list, const QSet<QString> &conflicts) {
        events = list;
        inConflict = conflicts;
        update();
    }
    void setMonth(const QDate &d) {
        month = QDate(d.year(), d.month(), 1);
        update();
    }
    QDate currentMonth() const { return month; }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        hits.clear();
        if (!month.isValid()) return;

        static const QStringList DAYS = {"Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"};
        const int lead = month.dayOfWeek() - 1;  // lundi en premier
        const QDate start = month.addDays(-lead);
        const int weeks = (lead + month.daysInMonth() + 6) / 7;
        const qreal headH = 28, colW = width() / 7.0, rowH = (height() - headH) / weeks;

        QFont f = font();
        f.setPixelSize(12);
        f.setBold(true);
        p.setFont(f);
        for (int d = 0; d < 7; ++d) {
            const QRectF h(d * colW, 0, colW, headH);
            p.fillRect(h, QColor("#0E1A2B"));
            p.setPen(Qt::white);
            p.drawText(h, Qt::AlignCenter, DAYS.at(d));
        }

        const qreal chipH = 18, gap = 3;
        for (int w = 0; w < weeks; ++w) {
            for (int d = 0; d < 7; ++d) {
                const QDate day = start.addDays(w * 7 + d);
                const QRectF cell(d * colW, headH + w * rowH, colW, rowH);
                const bool inMonth = day.month() == month.month();
                p.setPen(QPen(QColor("#E2E8F0"), 1));
                p.setBrush(!inMonth ? QColor("#F1F5F9") : (d >= 5 ? QColor("#F8FAFC") : QColor(Qt::white)));
                p.drawRect(cell);

                // numéro du jour (pastille orange pour aujourd'hui)
                const QRectF num(cell.left() + 6, cell.top() + 5, 24, 20);
                f.setPixelSize(12);
                f.setBold(true);
                p.setFont(f);
                if (day == QDate::currentDate()) {
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor("#FF6B00"));
                    p.drawRoundedRect(num, 10, 10);
                    p.setPen(Qt::white);
                } else {
                    p.setPen(inMonth ? QColor("#0F1B2D") : QColor("#94A3B8"));
                }
                p.drawText(num, Qt::AlignCenter, QString::number(day.day()));

                // compétitions de ce jour
                QVector<Comp> today;
                for (const Comp &c : events) {
                    const QDate a = QDate::fromString(c.debut, DATE_FMT), b = QDate::fromString(c.fin, DATE_FMT);
                    if (a.isValid() && b.isValid() && a <= day && day <= b) today.append(c);
                }
                const int room = int((rowH - 30 + gap) / (chipH + gap));
                const int shown = today.size() <= room ? int(today.size()) : qMax(0, room - 1);
                qreal y = cell.top() + 29;
                f.setPixelSize(11);
                p.setFont(f);
                for (int i = 0; i < shown; ++i) {
                    const Comp &c = today.at(i);
                    const QRectF chip(cell.left() + 4, y, colW - 8, chipH);
                    const bool conflict = inConflict.contains(c.id);
                    p.setPen(conflict ? QPen(QColor("#DC2626"), 1.5) : QPen(Qt::NoPen));
                    p.setBrush(QColor(statutBg(c.statut)));
                    p.drawRoundedRect(chip, 4, 4);
                    p.fillRect(QRectF(chip.left() + 1, chip.top() + 3, 3, chipH - 6), QColor(statutDot(c.statut)));
                    p.setPen(QColor(statutFg(c.statut)));
                    const QRectF text = chip.adjusted(8, 0, conflict ? -14 : -4, 0);
                    p.drawText(text, Qt::AlignLeft | Qt::AlignVCenter,
                               p.fontMetrics().elidedText(c.nom, Qt::ElideRight, int(text.width())));
                    if (conflict) {
                        p.setPen(Qt::NoPen);
                        p.setBrush(QColor("#DC2626"));
                        p.drawEllipse(QPointF(chip.right() - 7, chip.center().y()), 3.2, 3.2);
                    }
                    hits.append({chip, c});
                    y += chipH + gap;
                }
                if (shown < today.size()) {
                    p.setPen(QColor("#64748B"));
                    p.drawText(QRectF(cell.left() + 8, y, colW - 16, chipH), Qt::AlignLeft | Qt::AlignVCenter,
                               QString("+ %1 autre(s)").arg(today.size() - shown));
                }
            }
        }
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        setCursor(hitAt(e->position()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        if (const Comp *c = hitAt(e->position()))
            if (onPick) onPick(c->id);
    }

    bool event(QEvent *e) override {
        if (e->type() == QEvent::ToolTip) {
            auto *help = static_cast<QHelpEvent *>(e);
            if (const Comp *c = hitAt(QPointF(help->pos()))) {
                QString text = QString("<b>%1  %2</b><br>%3 · %4<br>Du %5 au %6<br>Salle : %7<br>Jury : %8")
                                   .arg(c->id, c->nom.toHtmlEscaped(), c->theme, c->mode, c->debut, c->fin,
                                        c->salle.isEmpty() ? QString("-") : c->salle.toHtmlEscaped(),
                                        c->jury.isEmpty() ? QString("-") : c->jury.toHtmlEscaped());
                if (inConflict.contains(c->id))
                    text += "<br><span style='color:#DC2626'><b>⛔ Conflit de planning</b></span>";
                QToolTip::showText(help->globalPos(), text, this);
            } else {
                QToolTip::hideText();
            }
            return true;
        }
        return QWidget::event(e);
    }

private:
    const Comp *hitAt(const QPointF &pos) const {
        for (const auto &h : hits)
            if (h.first.contains(pos)) return &h.second;
        return nullptr;
    }

    QVector<Comp> events;
    QSet<QString> inConflict;
    QDate month;
    QVector<QPair<QRectF, Comp>> hits;  // zones cliquables du dernier dessin
};

// ----------------------------------------------------------------- la fenêtre
class CalendarDialog : public QDialog {
public:
    CalendarDialog(QWidget *parent, const QVector<Comp> &events, const QSet<QString> &conflicts,
                   const QDate &startMonth)
        : QDialog(parent) {
        setWindowTitle("Calendrier des événements");
        resize(1040, 720);

        auto *prev = new QPushButton("‹");
        auto *next = new QPushButton("›");
        auto *todayBtn = new QPushButton("Aujourd'hui");
        for (QPushButton *b : {prev, next, todayBtn}) {
            b->setCursor(Qt::PointingHandCursor);
            b->setProperty("kind", "outline");
        }
        prev->setFixedWidth(44);
        next->setFixedWidth(44);
        title = new QLabel;
        title->setStyleSheet("font-size:22px; font-weight:800; color:#0F1B2D; padding:0 10px;");

        auto *legend = new QLabel(
            QString("<span style='color:%1'>■</span> Planifiée &nbsp;&nbsp;"
                    "<span style='color:%2'>■</span> En cours &nbsp;&nbsp;"
                    "<span style='color:%3'>■</span> Terminée &nbsp;&nbsp;"
                    "<span style='color:#DC2626'>●</span> Conflit de planning")
                .arg(statutDot("Planifiée"), statutDot("En cours"), statutDot("Terminée")));
        legend->setStyleSheet("color:#475569; font-size:12px;");

        auto *bar = new QHBoxLayout;
        bar->addWidget(prev);
        bar->addWidget(title);
        bar->addWidget(next);
        bar->addSpacing(8);
        bar->addWidget(todayBtn);
        bar->addStretch(1);
        bar->addWidget(legend);

        view = new MonthView;
        view->setEvents(events, conflicts);
        view->onPick = [this](const QString &id) {
            picked = id;
            accept();
        };

        auto *hint = new QLabel("Survolez une compétition pour voir ses détails, cliquez pour la sélectionner.");
        hint->setStyleSheet("color:#64748B; font-size:12px;");

        auto *lay = new QVBoxLayout(this);
        lay->addLayout(bar);
        lay->addWidget(view, 1);
        lay->addWidget(hint);

        connect(prev, &QPushButton::clicked, this, [this]() { showMonth(view->currentMonth().addMonths(-1)); });
        connect(next, &QPushButton::clicked, this, [this]() { showMonth(view->currentMonth().addMonths(1)); });
        connect(todayBtn, &QPushButton::clicked, this, [this]() { showMonth(QDate::currentDate()); });
        showMonth(startMonth.isValid() ? startMonth : QDate::currentDate());
    }

    QString pickedId() const { return picked; }  // vide si aucune compétition cliquée

private:
    void showMonth(const QDate &m) {
        view->setMonth(m);
        QString name = QLocale(QLocale::French).standaloneMonthName(m.month(), QLocale::LongFormat);
        if (!name.isEmpty()) name[0] = name[0].toUpper();
        title->setText(QString("%1 %2").arg(name).arg(m.year()));
    }

    MonthView *view;
    QLabel *title;
    QString picked;
};

#endif // CALENDARVIEW_H
