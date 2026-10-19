#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QWidget>

// Fait clignoter doucement un widget (le logo) en boucle. À lancer avec start().
inline QPropertyAnimation *makeBlink(QWidget *w, QObject *parent) {
    auto *fade = new QGraphicsOpacityEffect(w);
    w->setGraphicsEffect(fade);
    auto *blink = new QPropertyAnimation(fade, "opacity", parent);
    blink->setDuration(1400);
    blink->setKeyValueAt(0.0, 1.0);
    blink->setKeyValueAt(0.5, 0.25);
    blink->setKeyValueAt(1.0, 1.0);
    blink->setEasingCurve(QEasingCurve::InOutSine);
    blink->setLoopCount(-1);  // en boucle
    return blink;
}

// Agrandit un widget avec une animation quand la souris passe dessus
// (et, pour les champs de saisie, tant qu'on écrit dedans), puis le remet à sa taille.
class GrowOnHover : public QObject {
public:
    GrowOnHover(QWidget *w, QSize normal, QSize big, bool keepWhileFocused)
        : QObject(w), widget(w), normalSize(normal), bigSize(big), keepFocus(keepWhileFocused) {
        widget->setFixedSize(normalSize);
        anim = new QVariantAnimation(this);
        anim->setDuration(180);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QVariantAnimation::valueChanged, this,
                [this](const QVariant &v) { widget->setFixedSize(v.toSize()); });
        widget->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override {
        switch (ev->type()) {
        case QEvent::Enter:    hovered = true;  update(); break;
        case QEvent::Leave:    hovered = false; update(); break;
        case QEvent::FocusIn:
        case QEvent::FocusOut: update(); break;
        default: break;
        }
        return QObject::eventFilter(obj, ev);
    }

private:
    void update() {
        const bool big = hovered || (keepFocus && widget->hasFocus());
        const QSize target = big ? bigSize : normalSize;
        if (anim->endValue().toSize() == target && anim->state() == QAbstractAnimation::Running) return;
        anim->stop();
        anim->setStartValue(widget->size());
        anim->setEndValue(target);
        anim->start();
    }

    QWidget *widget;
    QSize normalSize, bigSize;
    bool keepFocus;
    bool hovered = false;
    QVariantAnimation *anim;
};

#endif // ANIMATIONS_H
