#include "connection.h"
#include "interface.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // couleurs de l'application (style.qss dans les ressources)
    QFile qss(":/style.qss");
    if (qss.open(QFile::ReadOnly))
        a.setStyleSheet(QString::fromUtf8(qss.readAll()));

    // connexion à la base Oracle : instance unique (Singleton)
    Connection &c = Connection::createInstance();
    if (c.createConnection())
        QMessageBox::information(nullptr, "Base de données", "Connexion réussie.");
    else
        QMessageBox::critical(nullptr, "Base de données",
                              "Échec de la connexion :\n" + c.lastError());

    // une seule fenêtre : la page de connexion, puis la gestion des compétitions
    Gcompetition w;
    w.setWindowIcon(QIcon(":/images/icon.png"));
    w.show();
    return a.exec();
}
