// Courbes des statistiques, dans la carte « Statistiques » de la colonne de droite.
// Dans hackathon.ui, le widget « statsChart » est promu vers cette classe (clic droit > Promote to...).
// Une courbe par série (équipes, participants…), un point par mois, la valeur au-dessus du point.
#ifndef STATSCHART_H
#define STATSCHART_H

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QStringList>
#include <QVector>
#include <QWidget>
#include <cmath>

class StatsChart : public QWidget {
public:
    struct Series {
        QString name;
        QColor color;
        QVector<double> values;  // une valeur par mois
    };

    explicit StatsChart(QWidget *parent = nullptr) : QWidget(parent) {}

    void setData(const QStringList &months, const QVector<Series> &series) {
        labels = months;
        lines = series;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor grid("#E5EAF3"), axis("#94A3B8"), grey("#64748B");

        // légende en haut
        QFont small = font();
        small.setPixelSize(11);
        p.setFont(small);
        qreal lx = 4;
        for (const Series &s : lines) {
            p.setPen(QPen(s.color, 2.4));
            p.drawLine(QPointF(lx, 9), QPointF(lx + 14, 9));
            p.setPen(grey);
            const qreal w = p.fontMetrics().horizontalAdvance(s.name);
            p.drawText(QRectF(lx + 18, 0, w + 2, 18), Qt::AlignLeft | Qt::AlignVCenter, s.name);
            lx += 18 + w + 14;
        }

        const QRectF plot(34, 30, width() - 44, height() - 52);
        if (labels.isEmpty()) {
            p.setPen(grey);
            p.drawText(plot, Qt::AlignCenter, "Aucune donnée");
            return;
        }

        // échelle verticale « ronde » : 1, 2, 5, 10, 20, 50…
        double maxValue = 1;
        for (const Series &s : lines)
            for (double v : s.values) maxValue = std::max(maxValue, v);
        const int ticks = 4;
        const double raw = std::max(1.0, maxValue / ticks);
        const double mag = std::pow(10.0, std::floor(std::log10(raw)));
        const double n = raw / mag;
        const double step = (n <= 1 ? 1 : n <= 2 ? 2 : n <= 5 ? 5 : 10) * mag, yMax = step * ticks;
        auto Y = [&](double v) { return plot.bottom() - plot.height() * v / yMax; };
        const int count = int(labels.size());
        auto X = [&](int i) { return count <= 1 ? plot.center().x() : plot.left() + plot.width() * i / (count - 1); };

        // grille, graduations, mois
        for (int t = 0; t <= ticks; ++t) {
            const double v = step * t;
            p.setPen(QPen(t == 0 ? axis : grid, t == 0 ? 1.0 : 0.8));
            p.drawLine(QPointF(plot.left(), Y(v)), QPointF(plot.right(), Y(v)));
            p.setPen(grey);
            p.drawText(QRectF(0, Y(v) - 8, 28, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(v));
        }
        const int every = std::max(1, (count + 4) / 5);  // au plus ~5 mois écrits sous l'axe
        for (int i = 0; i < count; ++i) {
            if (i % every != 0 && i != count - 1) continue;
            p.drawText(QRectF(X(i) - 32, plot.bottom() + 4, 64, 16), Qt::AlignCenter, labels[i]);
        }

        // courbes, points et valeurs
        for (const Series &s : lines) {
            QPainterPath path;
            for (int i = 0; i < count; ++i) {
                const QPointF pt(X(i), Y(s.values.value(i)));
                if (i == 0) path.moveTo(pt); else path.lineTo(pt);
            }
            p.setPen(QPen(s.color, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
            for (int i = 0; i < count; ++i) {
                const QPointF pt(X(i), Y(s.values.value(i)));
                p.setPen(QPen(Qt::white, 1.5));
                p.setBrush(s.color);
                p.drawEllipse(pt, 3.6, 3.6);
                if (count <= 8) {
                    p.setPen(s.color);
                    p.drawText(QRectF(pt.x() - 20, pt.y() - 19, 40, 14), Qt::AlignCenter,
                               QString::number(s.values.value(i)));
                }
            }
        }
    }

private:
    QStringList labels;
    QVector<Series> lines;
};

#endif // STATSCHART_H
