#ifndef LOGIN_H
#define LOGIN_H

#include <QPixmap>
#include <QWidget>

class QPropertyAnimation;

QT_BEGIN_NAMESPACE
namespace Ui { class Login; }
QT_END_NAMESPACE

// Page d'authentification : vérifie le nom d'utilisateur et le mot de passe,
// puis annonce le rôle de la personne connectée (Administrateur, Organisateur ou Jury).
class Login : public QWidget {
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login() override;
    void reset();   // vide les champs (après une déconnexion)

signals:
    void loggedIn(const QString &role);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void on_BLoginCompet_clicked();

private:
    Ui::Login *ui;
    QPixmap background, scaledBackground;
    QPropertyAnimation *blink;
};

#endif // LOGIN_H
