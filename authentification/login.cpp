#include "login.h"
#include "ui_login.h"
#include "animations.h"

// Les images (:/images/...) sont dans ressources.qrc du projet hackathon,
// compilé avec cette bibliothèque dans le même exécutable.

#include <QAction>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QIcon>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScreen>

// Comptes et rôles (plus tard : vérifiés dans la table EMPLOYE d'Oracle).
//   Administrateur : tout.
//   Organisateur   : ajouter, modifier, exporter, vérifier les conflits (pas de suppression).
//   Jury           : consulter et exporter seulement.
struct Account { QString user, password, role; };
static const QList<Account> ACCOUNTS = {
    {"hacktime123", "hacktime123", "Administrateur"},
    {"hacktime",    "hacktime",    "Organisateur"},
    {"hacktime321", "hacktime321", "Jury"},
};

Login::Login(QWidget *parent) : QWidget(parent), ui(new Ui::Login), background(":/images/login_bg.jpg") {
    ui->setupUi(this);

    QSize size(1536, 960);
    if (QScreen *screen = QGuiApplication::primaryScreen())
        size = size.boundedTo(screen->availableGeometry().size() - QSize(20, 50));
    resize(size);

    // texte d'aide des champs en bleu clair, lisible sur le fond sombre
    for (QLineEdit *e : {ui->EditLoginCompet, ui->EditPWDCompet}) {
        QPalette pal = e->palette();
        pal.setColor(QPalette::PlaceholderText, QColor(170, 192, 235));
        e->setPalette(pal);
    }

    // œil dans le mot de passe : afficher / masquer
    QAction *eye = ui->EditPWDCompet->addAction(QIcon(":/images/eye_off.png"), QLineEdit::TrailingPosition);
    eye->setToolTip("Afficher le mot de passe");
    connect(eye, &QAction::triggered, this, [this, eye]() {
        const bool hidden = ui->EditPWDCompet->echoMode() == QLineEdit::Password;
        ui->EditPWDCompet->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
        eye->setIcon(QIcon(hidden ? ":/images/eye.png" : ":/images/eye_off.png"));
        eye->setToolTip(hidden ? "Masquer le mot de passe" : "Afficher le mot de passe");
    });

    // champs et bouton qui s'agrandissent au survol ; la grille garde la grande taille
    // pour que le reste de la page ne bouge pas
    ui->loginFormLayout->setColumnMinimumWidth(1, 400);
    ui->loginFormLayout->setRowMinimumHeight(0, 52);
    ui->loginFormLayout->setRowMinimumHeight(1, 52);
    ui->loginFormLayout->setRowMinimumHeight(4, 58);
    new GrowOnHover(ui->EditLoginCompet, QSize(360, 46), QSize(400, 52), true);
    new GrowOnHover(ui->EditPWDCompet,   QSize(360, 46), QSize(400, 52), true);
    new GrowOnHover(ui->BLoginCompet,    QSize(170, 52), QSize(190, 58), false);

    blink = makeBlink(ui->logoLoginCompet, this);

    // Entrée : du nom d'utilisateur au mot de passe, puis connexion
    connect(ui->EditLoginCompet, &QLineEdit::returnPressed, this, [this]() { ui->EditPWDCompet->setFocus(); });
    connect(ui->EditPWDCompet, &QLineEdit::returnPressed, ui->BLoginCompet, &QPushButton::click);

    ui->lblErrLoginCompet->hide();
}

Login::~Login() {
    delete ui;
}

void Login::reset() {
    ui->EditLoginCompet->clear();
    ui->EditPWDCompet->clear();
    ui->EditPWDCompet->setEchoMode(QLineEdit::Password);
    ui->lblErrLoginCompet->hide();
    ui->EditLoginCompet->setFocus();
}

void Login::on_BLoginCompet_clicked() {
    const QString user = ui->EditLoginCompet->text().trimmed();
    const QString pwd = ui->EditPWDCompet->text();

    for (const Account &a : ACCOUNTS) {
        if (user == a.user && pwd == a.password) {
            ui->lblErrLoginCompet->hide();
            ui->EditPWDCompet->clear();
            emit loggedIn(a.role);
            return;
        }
    }

    ui->lblErrLoginCompet->setText(user.isEmpty() || pwd.isEmpty()
                                       ? "Saisissez le nom d'utilisateur et le mot de passe."
                                       : "Nom d'utilisateur ou mot de passe incorrect.");
    ui->lblErrLoginCompet->show();
    ui->EditPWDCompet->clear();
    ui->EditPWDCompet->setFocus();
}

// fond « circuits » qui couvre toute la fenêtre sans être déformé
void Login::paintEvent(QPaintEvent *) {
    if (scaledBackground.isNull() || scaledBackground.size() != background.size().scaled(size(), Qt::KeepAspectRatioByExpanding))
        scaledBackground = background.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPainter p(this);
    p.drawPixmap((width() - scaledBackground.width()) / 2, (height() - scaledBackground.height()) / 2, scaledBackground);
}

// le logo ne clignote que lorsque la page est affichée
void Login::showEvent(QShowEvent *event) {
    blink->start();
    QWidget::showEvent(event);
}

void Login::hideEvent(QHideEvent *event) {
    blink->stop();
    static_cast<QGraphicsOpacityEffect *>(blink->targetObject())->setOpacity(1.0);
    QWidget::hideEvent(event);
}
