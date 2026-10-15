#include "hackathon.h"
#include "login.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // style de toute l'application
    QFile f(":/style.qss");
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream ts(&f);
        a.setStyleSheet(ts.readAll());
    }

    Login login;
    hackathon HT;
    login.setWindowIcon(QIcon(":/images/icon.png"));
    HT.setWindowIcon(QIcon(":/images/icon.png"));

    // connexion réussie -> interface du projet avec les droits du rôle
    QObject::connect(&login, &Login::loggedIn, &HT, [&](const QString &role) {
        HT.setRole(role);
        HT.show();
        login.hide();
    });

    // déconnexion -> retour à la page d'authentification
    QObject::connect(&HT, &hackathon::loggedOut, &login, [&]() {
        HT.hide();
        login.reset();
        login.show();
    });

    login.show();
    return QApplication::exec();
}
