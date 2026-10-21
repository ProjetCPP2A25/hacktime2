#include "gcompetition.h"
#include "login.h"
#include "connection.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
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

    // connexion à la base Oracle : instance unique (patron Singleton, voir connection.cpp)
    Connection &c = Connection::createInstance();
    if (c.createConnection())
        QMessageBox::information(nullptr, "Base de données", "Connexion réussie.");
    else
        QMessageBox::critical(nullptr, "Base de données", "Échec de la connexion :\n" + c.lastError());

    Login login;
    Gcompetition HT;
    login.setWindowIcon(QIcon(":/images/icon.png"));
    HT.setWindowIcon(QIcon(":/images/icon.png"));

    // connexion réussie -> interface du projet avec les droits du rôle
    QObject::connect(&login, &Login::loggedIn, &HT, [&](const QString &role) {
        HT.setRole(role);
        HT.show();
        login.hide();
    });

    // déconnexion -> retour à la page d'authentification
    QObject::connect(&HT, &Gcompetition::loggedOut, &login, [&]() {
        HT.hide();
        login.reset();
        login.show();
    });

    login.show();
    return QApplication::exec();
}
