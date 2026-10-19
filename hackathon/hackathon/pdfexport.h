// Export PDF du module Compétitions (A4 paysage) : planning des sponsors.
// Chaque sponsor, les compétitions qu'il soutient, avec thème et dates :
// d'abord une frise dans le temps, puis un tableau détaillé par sponsor.
// Le PDF est écrit à 72 dpi : 1 unité = 1 point (A4 paysage = 842 x 595).
#ifndef PDFEXPORT_H
#define PDFEXPORT_H

#include <QImage>
#include <QPainter>
#include <QPdfWriter>
#include <QVector>
#include "competition.h"

namespace pdf {

// Dessine tout le rapport, sur autant de pages que nécessaire.
// « events » = compétitions exportées (filtres et recherche de l'application compris).
void drawReport(QPdfWriter &writer, QPainter &p, const QVector<Comp> &events, const QImage &logo);

}  // namespace pdf

#endif // PDFEXPORT_H
