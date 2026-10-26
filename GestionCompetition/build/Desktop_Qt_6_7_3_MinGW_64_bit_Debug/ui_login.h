/********************************************************************************
** Form generated from reading UI file 'login.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGIN_H
#define UI_LOGIN_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Login
{
public:
    QVBoxLayout *loginLayout;
    QHBoxLayout *loginTopLayout;
    QLabel *logoLoginCompet;
    QSpacerItem *loginTopSpacer;
    QSpacerItem *loginSpacerHaut;
    QHBoxLayout *loginCenterLayout;
    QSpacerItem *loginSpacerGauche;
    QFrame *cardLoginCompet;
    QVBoxLayout *cardLayout;
    QSpacerItem *cardSpacerHaut;
    QHBoxLayout *cardFormRow;
    QSpacerItem *cardSpacerGauche;
    QGridLayout *loginFormLayout;
    QLabel *lblLoginCompet;
    QLineEdit *EditLoginCompet;
    QLabel *lblPWDCompet;
    QLineEdit *EditPWDCompet;
    QLabel *lblErrLoginCompet;
    QPushButton *BForgotCompet;
    QPushButton *BLoginCompet;
    QSpacerItem *cardSpacerDroite;
    QSpacerItem *cardSpacerBas;
    QSpacerItem *loginSpacerDroite;
    QSpacerItem *loginSpacerBas;

    void setupUi(QWidget *Login)
    {
        if (Login->objectName().isEmpty())
            Login->setObjectName("Login");
        Login->resize(1280, 800);
        loginLayout = new QVBoxLayout(Login);
        loginLayout->setSpacing(0);
        loginLayout->setObjectName("loginLayout");
        loginLayout->setContentsMargins(32, 24, 32, 32);
        loginTopLayout = new QHBoxLayout();
        loginTopLayout->setObjectName("loginTopLayout");
        logoLoginCompet = new QLabel(Login);
        logoLoginCompet->setObjectName("logoLoginCompet");
        logoLoginCompet->setMinimumSize(QSize(380, 108));
        logoLoginCompet->setMaximumSize(QSize(380, 108));
        logoLoginCompet->setPixmap(QPixmap(QString::fromUtf8(":/images/logo_hacktime_clair.png")));
        logoLoginCompet->setScaledContents(true);

        loginTopLayout->addWidget(logoLoginCompet);

        loginTopSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        loginTopLayout->addItem(loginTopSpacer);


        loginLayout->addLayout(loginTopLayout);

        loginSpacerHaut = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        loginLayout->addItem(loginSpacerHaut);

        loginCenterLayout = new QHBoxLayout();
        loginCenterLayout->setObjectName("loginCenterLayout");
        loginSpacerGauche = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        loginCenterLayout->addItem(loginSpacerGauche);

        cardLoginCompet = new QFrame(Login);
        cardLoginCompet->setObjectName("cardLoginCompet");
        cardLoginCompet->setMinimumSize(QSize(860, 540));
        cardLoginCompet->setMaximumSize(QSize(860, 540));
        cardLayout = new QVBoxLayout(cardLoginCompet);
        cardLayout->setSpacing(0);
        cardLayout->setObjectName("cardLayout");
        cardLayout->setContentsMargins(45, 45, 45, 45);
        cardSpacerHaut = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        cardLayout->addItem(cardSpacerHaut);

        cardFormRow = new QHBoxLayout();
        cardFormRow->setObjectName("cardFormRow");
        cardSpacerGauche = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        cardFormRow->addItem(cardSpacerGauche);

        loginFormLayout = new QGridLayout();
        loginFormLayout->setObjectName("loginFormLayout");
        loginFormLayout->setHorizontalSpacing(40);
        loginFormLayout->setVerticalSpacing(18);
        lblLoginCompet = new QLabel(cardLoginCompet);
        lblLoginCompet->setObjectName("lblLoginCompet");

        loginFormLayout->addWidget(lblLoginCompet, 0, 0, 1, 1);

        EditLoginCompet = new QLineEdit(cardLoginCompet);
        EditLoginCompet->setObjectName("EditLoginCompet");
        EditLoginCompet->setMinimumSize(QSize(280, 0));

        loginFormLayout->addWidget(EditLoginCompet, 0, 1, 1, 1, Qt::AlignCenter);

        lblPWDCompet = new QLabel(cardLoginCompet);
        lblPWDCompet->setObjectName("lblPWDCompet");

        loginFormLayout->addWidget(lblPWDCompet, 1, 0, 1, 1);

        EditPWDCompet = new QLineEdit(cardLoginCompet);
        EditPWDCompet->setObjectName("EditPWDCompet");
        EditPWDCompet->setMinimumSize(QSize(280, 0));
        EditPWDCompet->setEchoMode(QLineEdit::Password);

        loginFormLayout->addWidget(EditPWDCompet, 1, 1, 1, 1, Qt::AlignCenter);

        lblErrLoginCompet = new QLabel(cardLoginCompet);
        lblErrLoginCompet->setObjectName("lblErrLoginCompet");

        loginFormLayout->addWidget(lblErrLoginCompet, 2, 1, 1, 1, Qt::AlignHCenter);

        BForgotCompet = new QPushButton(cardLoginCompet);
        BForgotCompet->setObjectName("BForgotCompet");
        BForgotCompet->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        BForgotCompet->setFlat(true);

        loginFormLayout->addWidget(BForgotCompet, 3, 1, 1, 1, Qt::AlignHCenter);

        BLoginCompet = new QPushButton(cardLoginCompet);
        BLoginCompet->setObjectName("BLoginCompet");
        BLoginCompet->setMinimumSize(QSize(120, 0));
        BLoginCompet->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        BLoginCompet->setProperty("kind", QVariant(QString::fromUtf8("primary")));

        loginFormLayout->addWidget(BLoginCompet, 4, 1, 1, 1, Qt::AlignCenter);


        cardFormRow->addLayout(loginFormLayout);

        cardSpacerDroite = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        cardFormRow->addItem(cardSpacerDroite);


        cardLayout->addLayout(cardFormRow);

        cardSpacerBas = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        cardLayout->addItem(cardSpacerBas);


        loginCenterLayout->addWidget(cardLoginCompet);

        loginSpacerDroite = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        loginCenterLayout->addItem(loginSpacerDroite);


        loginLayout->addLayout(loginCenterLayout);

        loginSpacerBas = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        loginLayout->addItem(loginSpacerBas);

        loginLayout->setStretch(1, 1);
        loginLayout->setStretch(3, 2);

        retranslateUi(Login);

        QMetaObject::connectSlotsByName(Login);
    } // setupUi

    void retranslateUi(QWidget *Login)
    {
        Login->setWindowTitle(QCoreApplication::translate("Login", "HackTime - Connexion", nullptr));
        logoLoginCompet->setText(QString());
        lblLoginCompet->setText(QCoreApplication::translate("Login", "Nom d'utilisateur", nullptr));
        EditLoginCompet->setPlaceholderText(QCoreApplication::translate("Login", "Entrez votre nom d'utilisateur", nullptr));
        lblPWDCompet->setText(QCoreApplication::translate("Login", "Mot de passe", nullptr));
        EditPWDCompet->setPlaceholderText(QCoreApplication::translate("Login", "Entrez votre mot de passe", nullptr));
        lblErrLoginCompet->setText(QCoreApplication::translate("Login", "Nom d'utilisateur ou mot de passe incorrect.", nullptr));
        BForgotCompet->setText(QCoreApplication::translate("Login", "Mot de passe oubli\303\251 ?", nullptr));
        BLoginCompet->setText(QCoreApplication::translate("Login", "Entrer", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Login: public Ui_Login {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGIN_H
