#include "interface.h"
#include "ui_interface.h"
#include "ui_login.h"

#include <QAction>
#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScreen>
#include <QSignalBlocker>
#include <QTableWidgetItem>
#include <QVariantAnimation>
#include <algorithm>
#include <cmath>

// =====================================================================
//  Données fixes de l'interface
// =====================================================================
static const int PAR_PAGE = 8;

static const QStringList THEMES = {"Environnement", "Santé", "Éducation", "Ville intelligente",
                                   "Agriculture", "Intelligence artificielle", "Développement",
                                   "Finance", "Autre"};
static const QStringList MODES = {"Présentiel", "En ligne", "Hybride"};
static const QStringList STATUTS = {"Planifiée", "En cours", "Terminée"};
static const QStringList SALLES = {"Amphi 1", "Amphi 2", "Salle A", "Salle B",
                                   "Lab Innovation", "Espace Coworking"};

static QString themeIcon(const QString &t)
{
    if (t == "Environnement") return "🍃";
    if (t == "Santé" || t == "Sante") return "❤️";
    if (t == "Éducation") return "🎓";
    if (t == "Ville intelligente") return "🏙️";
    if (t == "Agriculture") return "🌱";
    if (t == "Intelligence artificielle") return "🤖";
    if (t == "Développement") return "</>";
    if (t == "Finance") return "🪙";
    return "⭐";
}
static QString modeIcon(const QString &m)
{
    if (m == "En ligne") return "💻";
    if (m == "Hybride") return "🔀";
    return "🏢";
}
static QString statutBg(const QString &s)
{
    if (s == "En cours") return "#DDF3E4";
    if (s == "Terminée") return "#E5E9F0";
    return "#DCE9FF";
}
static QString statutFg(const QString &s)
{
    if (s == "En cours") return "#2E9E5B";
    if (s == "Terminée") return "#64748B";
    return "#2563EB";
}
static const QStringList VIDE = {"", "", "", ""};   // salle, jury, description, sponsors
static QString ouTiret(const QString &s) { return s.isEmpty() ? QString("-") : s; }

// le logo clignote doucement, en boucle
static QPropertyAnimation *clignoter(QWidget *w, QObject *parent)
{
    auto *effet = new QGraphicsOpacityEffect(w);
    w->setGraphicsEffect(effet);
    auto *anim = new QPropertyAnimation(effet, "opacity", parent);
    anim->setDuration(1400);
    anim->setKeyValueAt(0.0, 1.0);
    anim->setKeyValueAt(0.5, 0.25);
    anim->setKeyValueAt(1.0, 1.0);
    anim->setEasingCurve(QEasingCurve::InOutSine);
    anim->setLoopCount(-1);
    return anim;
}

// agrandit / réduit un widget avec une petite animation
static void animerTaille(QWidget *w, const QSize &cible)
{
    auto *anim = new QVariantAnimation(w);
    anim->setDuration(180);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setStartValue(w->size());
    anim->setEndValue(cible);
    QObject::connect(anim, &QVariantAnimation::valueChanged, w,
                     [w](const QVariant &v) { w->setFixedSize(v.toSize()); });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// =====================================================================
//  Constructeur
// =====================================================================
Gcompetition::Gcompetition(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::Gcompetition), uiLogin(new Ui::Login),
      fondLogin(":/images/login_bg.jpg")
{
    ui->setupUi(this);      // interface.ui : la gestion des compétitions

    // login.ui : la page de connexion, placée en premier dans la pile de pages
    pageLogin = new QWidget;
    uiLogin->setupUi(pageLogin);
    ui->SWHackTime->insertWidget(0, pageLogin);

    // la fenêtre ne dépasse jamais l'écran
    QSize taille(1536, 960);
    if (QScreen *ecran = QGuiApplication::primaryScreen())
        taille = taille.boundedTo(ecran->availableGeometry().size() - QSize(20, 50));
    resize(taille);

    preparerLogin();
    preparerFenetre();
    afficherLogin();
}

Gcompetition::~Gcompetition()
{
    delete uiLogin;
    delete ui;
    ui = nullptr;   // eventFilter ne doit plus utiliser l'interface
}

// =====================================================================
//  LOGIN (page 0)
// =====================================================================
void Gcompetition::preparerLogin()
{
    // texte d'aide en bleu clair, lisible sur le fond sombre
    for (QLineEdit *e : {uiLogin->EditLoginCompet, uiLogin->EditPWDCompet}) {
        QPalette pal = e->palette();
        pal.setColor(QPalette::PlaceholderText, QColor(170, 192, 235));
        e->setPalette(pal);
    }

    // œil dans le mot de passe : afficher / masquer
    QAction *oeil = uiLogin->EditPWDCompet->addAction(QIcon(":/images/eye_off.png"), QLineEdit::TrailingPosition);
    oeil->setToolTip("Afficher le mot de passe");
    connect(oeil, &QAction::triggered, this, [this, oeil]() {
        const bool cache = uiLogin->EditPWDCompet->echoMode() == QLineEdit::Password;
        uiLogin->EditPWDCompet->setEchoMode(cache ? QLineEdit::Normal : QLineEdit::Password);
        oeil->setIcon(QIcon(cache ? ":/images/eye.png" : ":/images/eye_off.png"));
        oeil->setToolTip(cache ? "Masquer le mot de passe" : "Afficher le mot de passe");
    });

    // champs et bouton qui s'agrandissent au survol (voir eventFilter)
    uiLogin->loginFormLayout->setColumnMinimumWidth(1, 400);
    uiLogin->loginFormLayout->setRowMinimumHeight(0, 52);
    uiLogin->loginFormLayout->setRowMinimumHeight(1, 52);
    uiLogin->loginFormLayout->setRowMinimumHeight(4, 58);
    uiLogin->EditLoginCompet->setFixedSize(360, 46);
    uiLogin->EditPWDCompet->setFixedSize(360, 46);
    uiLogin->BLoginCompet->setFixedSize(170, 52);
    for (QWidget *w : {(QWidget *)uiLogin->EditLoginCompet, (QWidget *)uiLogin->EditPWDCompet,
                       (QWidget *)uiLogin->BLoginCompet})
        w->installEventFilter(this);

    // image de fond dessinée dans eventFilter
    pageLogin->installEventFilter(this);
    clignotementLogin = clignoter(uiLogin->logoLoginCompet, this);

    // Entrée : du nom d'utilisateur au mot de passe, puis connexion
    connect(uiLogin->EditLoginCompet, &QLineEdit::returnPressed, this, [this]() { uiLogin->EditPWDCompet->setFocus(); });
    connect(uiLogin->EditPWDCompet, &QLineEdit::returnPressed, uiLogin->BLoginCompet, &QPushButton::click);
    connect(uiLogin->BLoginCompet, &QPushButton::clicked, this, &Gcompetition::seConnecter);
}

void Gcompetition::afficherLogin()
{
    uiLogin->EditLoginCompet->clear();
    uiLogin->EditPWDCompet->clear();
    uiLogin->EditPWDCompet->setEchoMode(QLineEdit::Password);
    uiLogin->lblErrLoginCompet->hide();
    ui->SWHackTime->setCurrentWidget(pageLogin);
    setWindowTitle("HackTime - Connexion");
    clignotementLogin->start();
    uiLogin->EditLoginCompet->setFocus();
}

// Login d'affichage seulement : aucune vérification, n'importe quel nom
// d'utilisateur et mot de passe (même vides) ouvrent l'application en Administrateur.
void Gcompetition::seConnecter()
{
    role = "Administrateur";
    clignotementLogin->stop();
    static_cast<QGraphicsOpacityEffect *>(clignotementLogin->targetObject())->setOpacity(1.0);

    ui->admin->setText(QString("👤  <b>%1</b>  ▾").arg(role));
    ui->admin->setToolTip("Cliquer pour se déconnecter");
    ui->cardAdd->setVisible(true);
    on_btnClear_clicked();

    ui->SWHackTime->setCurrentWidget(ui->PComp);
    setWindowTitle("HackTime");
    afficher();
}

void Gcompetition::deconnexion()
{
    if (QMessageBox::question(this, "Déconnexion", "Se déconnecter de HackTime ?") != QMessageBox::Yes)
        return;
    role.clear();
    afficherLogin();
}

bool Gcompetition::peutModifier() const { return role == "Administrateur" || role == "Organisateur"; }
bool Gcompetition::peutSupprimer() const { return role == "Administrateur"; }

// =====================================================================
//  Événements : fond du login, survol, courbes, clic sur le nom, molette
// =====================================================================
bool Gcompetition::eventFilter(QObject *obj, QEvent *ev)
{
    if (!ui) return false;   // fenêtre en cours de fermeture

    // fond « circuits » de la page de connexion, sans déformation
    if (obj == pageLogin && ev->type() == QEvent::Paint) {
        QWidget *p = pageLogin;
        const QSize voulue = fondLogin.size().scaled(p->size(), Qt::KeepAspectRatioByExpanding);
        if (fondLoginAjuste.size() != voulue)
            fondLoginAjuste = fondLogin.scaled(p->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QPainter painter(p);
        painter.drawPixmap((p->width() - fondLoginAjuste.width()) / 2,
                           (p->height() - fondLoginAjuste.height()) / 2, fondLoginAjuste);
        return true;
    }

    // champs et bouton du login : plus grands au survol (et pendant la saisie)
    if (obj == uiLogin->EditLoginCompet || obj == uiLogin->EditPWDCompet || obj == uiLogin->BLoginCompet) {
        QWidget *w = static_cast<QWidget *>(obj);
        const bool estBouton = (obj == uiLogin->BLoginCompet);
        if (ev->type() == QEvent::Enter) w->setProperty("survol", true);
        if (ev->type() == QEvent::Leave) w->setProperty("survol", false);
        if (ev->type() == QEvent::Enter || ev->type() == QEvent::Leave
            || ev->type() == QEvent::FocusIn || ev->type() == QEvent::FocusOut) {
            const bool grand = w->property("survol").toBool() || (!estBouton && w->hasFocus());
            const QSize cible = estBouton ? (grand ? QSize(190, 58) : QSize(170, 52))
                                          : (grand ? QSize(400, 52) : QSize(360, 46));
            if (w->size() != cible) animerTaille(w, cible);
        }
        return QMainWindow::eventFilter(obj, ev);
    }

    // courbes des statistiques, dessinées dans le QLabel « statsChart »
    if (obj == ui->statsChart && ev->type() == QEvent::Paint) {
        dessinerCourbes(ui->statsChart);
        return true;
    }

    // clic sur le nom en haut à droite -> déconnexion
    if (obj == ui->admin) {
        if (ev->type() == QEvent::MouseButtonPress) {
            deconnexion();
            return true;
        }
        return QMainWindow::eventFilter(obj, ev);
    }

    // molette sur un champ du formulaire : fait défiler la colonne au lieu de changer la valeur
    if (ev->type() == QEvent::Wheel) {
        ev->ignore();
        return true;
    }
    return QMainWindow::eventFilter(obj, ev);
}

// =====================================================================
//  Mise en place de la page Compétitions (page 1)
// =====================================================================
void Gcompetition::preparerFenetre()
{
    ui->SWCompet->setCurrentIndex(0);
    ui->admin->setCursor(Qt::PointingHandCursor);
    ui->admin->installEventFilter(this);
    ui->rightScroll->viewport()->setAutoFillBackground(false);
    ui->rightCol->setAutoFillBackground(false);
    clignoter(ui->logo, this)->start();
    ui->statsChart->setMinimumHeight(180);
    ui->statsChart->installEventFilter(this);

    // tableau
    QTableWidget *t = ui->table;
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    const int largeurs[] = {70, 0, 170, 100, 100, 110, 100, 100, 120};
    for (int i = 0; i < t->columnCount() && i < 9; ++i)
        if (largeurs[i]) t->setColumnWidth(i, largeurs[i]);
    connect(t, &QTableWidget::itemSelectionChanged, this, [this]() {
        const auto lignes = ui->table->selectionModel()->selectedRows();
        if (lignes.isEmpty()) return;
        idSelectionne = ui->table->item(lignes.first().row(), 0)->text().toInt();
        afficherDetails();
    });

    // MÉTIER : recherche, filtres et tri
    ui->fStatut->addItem("Tous");  ui->fStatut->addItems(STATUTS);
    ui->fTheme->addItem("Tous");   ui->fTheme->addItems(THEMES);
    ui->fDate->addItem("Toutes");
    ui->fSort->addItems({"Date début ↑", "Date début ↓", "Date fin ↑", "Date fin ↓", "ID"});
    // la recherche du haut et celle de la liste écrivent dans le même champ
    connect(ui->topSearch, &QLineEdit::textChanged, ui->search, &QLineEdit::setText);
    connect(ui->search, &QLineEdit::textChanged, this, [this]() { page = 0; appliquerFiltres(); });
    for (QComboBox *c : {ui->fStatut, ui->fTheme, ui->fDate, ui->fSort})
        connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](int) { page = 0; appliquerFiltres(); });

    // formulaire d'ajout / de modification
    ui->aTheme->addItems(THEMES);
    ui->aMode->addItems(MODES);
    ui->aStatut->addItems(STATUTS);
    ui->aSalle->addItems(SALLES);
    ui->aConflict->hide();
    for (QWidget *f : {(QWidget *)ui->aTheme, (QWidget *)ui->aDebut, (QWidget *)ui->aFin,
                       (QWidget *)ui->aMode, (QWidget *)ui->aMax, (QWidget *)ui->aSalle,
                       (QWidget *)ui->aStatut}) {
        f->setFocusPolicy(Qt::StrongFocus);
        f->installEventFilter(this);
    }
    connect(ui->aNom, &QLineEdit::returnPressed, this, &Gcompetition::on_btnSave_clicked);
    connect(ui->aMode, &QComboBox::currentTextChanged, this,
            [this](const QString &m) { ui->aSalle->setEnabled(m != "En ligne"); });

    // boutons d'affichage seulement : le clic ne fait rien (on reste sur « Compétitions »)
    for (QPushButton *b : {ui->btnDashboard, ui->btnEquipes, ui->btnParticipants, ui->btnAtelier,
                           ui->btnEmployes, ui->btnParametres})
        connect(b, &QPushButton::clicked, this, [this]() { ui->btnCompetitions->setChecked(true); });
    on_btnClear_clicked();
}

// =====================================================================
//  AFFICHER (lecture dans la base)
// =====================================================================
void Gcompetition::afficher()
{
    liste = Competition::lister();
    chargerInfos();
    if (indexDe(idSelectionne) < 0)
        idSelectionne = liste.isEmpty() ? 0 : liste.first().getId();
    remplirAnnees();
    appliquerFiltres();
    mettreAJourStatistiques();
    afficherDetails();
}

// =====================================================================
//  MÉTIER : recherche, filtres et tri
// =====================================================================
// garde dans « filtree » les compétitions qui correspondent à la recherche
// et aux filtres, triées selon « Trier par », puis remplit le tableau
void Gcompetition::appliquerFiltres()
{
    const QString recherche = ui->search->text().trimmed().toLower();
    const QString statut = ui->fStatut->currentText();
    const QString theme = ui->fTheme->currentText();
    const QString annee = ui->fDate->currentText();

    filtree.clear();
    for (const Competition &c : liste) {
        const QStringList x = infos.value(c.getId(), VIDE);
        // recherche : ID, nom, thème, mode, salle, jury, description ou sponsors
        const QString texte = QStringList({QString::number(c.getId()), c.getNom(), c.getTheme(),
                                           c.getMode(), x.value(0), x.value(1), x.value(2), x.value(3)})
                                  .join(" ").toLower();
        if (!recherche.isEmpty() && !texte.contains(recherche)) continue;
        if (statut != "Tous" && c.getStatut() != statut) continue;
        if (theme != "Tous" && c.getTheme() != theme) continue;
        if (annee != "Toutes" && QString::number(c.getDateDebut().year()) != annee) continue;
        filtree.append(c);
    }

    const int tri = ui->fSort->currentIndex();
    std::stable_sort(filtree.begin(), filtree.end(), [tri](const Competition &a, const Competition &b) {
        switch (tri) {
        case 0: return a.getDateDebut() < b.getDateDebut();   // Date début ↑
        case 1: return a.getDateDebut() > b.getDateDebut();   // Date début ↓
        case 2: return a.getDateFin() < b.getDateFin();       // Date fin ↑
        case 3: return a.getDateFin() > b.getDateFin();       // Date fin ↓
        default: return a.getId() < b.getId();                // ID
        }
    });

    const int nbPages = std::max(1, (int(filtree.size()) + PAR_PAGE - 1) / PAR_PAGE);
    page = std::max(0, std::min(page, nbPages - 1));
    remplirTableau();
}

// filtre « Année » : les années de début présentes dans la base
void Gcompetition::remplirAnnees()
{
    const QString actuelle = ui->fDate->currentText();
    QStringList annees;
    for (const Competition &c : liste) {
        const QString a = QString::number(c.getDateDebut().year());
        if (!annees.contains(a)) annees << a;
    }
    annees.sort();
    QSignalBlocker bloque(ui->fDate);
    ui->fDate->clear();
    ui->fDate->addItem("Toutes");
    ui->fDate->addItems(annees);
    const int i = ui->fDate->findText(actuelle);
    ui->fDate->setCurrentIndex(i >= 0 ? i : 0);
}

// remet la recherche et les filtres à zéro (après un ajout)
void Gcompetition::reinitialiserFiltres()
{
    QSignalBlocker b1(ui->search), b2(ui->topSearch), b3(ui->fStatut), b4(ui->fTheme), b5(ui->fDate);
    ui->search->clear();
    ui->topSearch->clear();
    ui->fStatut->setCurrentIndex(0);
    ui->fTheme->setCurrentIndex(0);
    ui->fDate->setCurrentIndex(0);
}

void Gcompetition::remplirTableau()
{
    const int nbPages = std::max(1, (int(filtree.size()) + PAR_PAGE - 1) / PAR_PAGE);
    const int debut = page * PAR_PAGE;
    const int nb = std::max(0, std::min(PAR_PAGE, int(filtree.size()) - debut));

    QTableWidget *t = ui->table;
    {
        QSignalBlocker bloque(t);
        t->clearContents();
        t->setRowCount(nb);
        for (int r = 0; r < nb; ++r) {
            const Competition &c = filtree.at(debut + r);
            auto cellule = [](const QString &texte, bool centre = false) {
                QTableWidgetItem *it = new QTableWidgetItem(texte);
                if (centre) it->setTextAlignment(Qt::AlignCenter);
                return it;
            };
            QTableWidgetItem *idItem = cellule(QString::number(c.getId()));
            idItem->setForeground(QColor("#1665E8"));
            t->setItem(r, 0, idItem);
            t->setItem(r, 1, cellule(c.getNom()));
            t->setItem(r, 2, cellule(themeIcon(c.getTheme()) + "  " + c.getTheme()));
            t->setItem(r, 3, cellule(c.getDateDebut().toString("dd/MM/yyyy")));
            t->setItem(r, 4, cellule(c.getDateFin().toString("dd/MM/yyyy")));
            t->setItem(r, 5, cellule(modeIcon(c.getMode()) + "  " + c.getMode()));
            t->setItem(r, 6, cellule(QString::number(c.getNbMaxMembres()), true));
            t->setCellWidget(r, 7, creerBadge(c.getStatut()));
            t->setCellWidget(r, 8, creerActions(c.getId()));
            if (c.getId() == idSelectionne) t->selectRow(r);
        }
    }
    const int total = int(filtree.size());
    ui->footer->setText(QString("Affichage de %1 à %2 sur %3 compétitions")
                            .arg(total ? debut + 1 : 0).arg(debut + nb).arg(total));
    construirePages(nbPages);
}

// boutons ‹ 1 2 3 › sous le tableau
void Gcompetition::construirePages(int nbPages)
{
    while (QLayoutItem *it = ui->pagerLayout->takeAt(0)) {
        if (QWidget *w = it->widget()) { w->hide(); w->deleteLater(); }
        delete it;
    }
    auto aller = [this](int p) { page = p; remplirTableau(); };
    QPushButton *prec = new QPushButton("‹");
    prec->setObjectName("page");
    prec->setEnabled(page > 0);
    connect(prec, &QPushButton::clicked, this, [this, aller]() { aller(page - 1); });
    ui->pagerLayout->addWidget(prec);
    for (int p = 0; p < nbPages; ++p) {
        QPushButton *b = new QPushButton(QString::number(p + 1));
        b->setObjectName(p == page ? "pageOn" : "page");
        connect(b, &QPushButton::clicked, this, [aller, p]() { aller(p); });
        ui->pagerLayout->addWidget(b);
    }
    QPushButton *suiv = new QPushButton("›");
    suiv->setObjectName("page");
    suiv->setEnabled(page < nbPages - 1);
    connect(suiv, &QPushButton::clicked, this, [this, aller]() { aller(page + 1); });
    ui->pagerLayout->addWidget(suiv);
}

QWidget *Gcompetition::creerBadge(const QString &statut)
{
    QLabel *lab = new QLabel(statut);
    lab->setAlignment(Qt::AlignCenter);
    lab->setStyleSheet(QString("background:%1;color:%2;border-radius:11px;padding:3px 12px;"
                               "font-size:12px;").arg(statutBg(statut), statutFg(statut)));
    QWidget *w = new QWidget;
    QHBoxLayout *l = new QHBoxLayout(w);
    l->setContentsMargins(4, 0, 4, 0);
    l->addWidget(lab);
    return w;
}

// 👁 voir   ✏️ modifier   🗑️ supprimer
QWidget *Gcompetition::creerActions(int id)
{
    QWidget *w = new QWidget;
    QHBoxLayout *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    QPushButton *voir = new QPushButton("👁"), *modif = new QPushButton("✏️"), *suppr = new QPushButton("🗑️");
    voir->setToolTip("Voir les détails");
    modif->setToolTip("Modifier");
    suppr->setToolTip("Supprimer");
    for (QPushButton *b : {voir, modif, suppr}) {
        b->setObjectName("act");
        b->setCursor(Qt::PointingHandCursor);
        l->addWidget(b);
    }
    modif->setVisible(peutModifier());    // le jury ne modifie pas
    suppr->setVisible(peutSupprimer());   // seul l'administrateur supprime
    connect(voir, &QPushButton::clicked, this, [this, id]() {
        idSelectionne = id;
        afficherDetails();
    });
    connect(modif, &QPushButton::clicked, this, [this, id]() { chargerFormulaire(id); });
    connect(suppr, &QPushButton::clicked, this, [this, id]() { supprimer(id); });
    return w;
}

int Gcompetition::indexDe(int id) const
{
    for (int i = 0; i < liste.size(); ++i)
        if (liste.at(i).getId() == id) return i;
    return -1;
}

// =====================================================================
//  AJOUTER / MODIFIER (formulaire de la colonne droite)
// =====================================================================
Competition Gcompetition::lireFormulaire() const
{
    Competition c(ui->aNom->text(), ui->aTheme->currentText(), ui->aDebut->date(),
                  ui->aFin->date(), ui->aMax->value(), ui->aStatut->currentText(),
                  ui->aMode->currentText());
    c.setId(idEnModification);
    return c;
}

void Gcompetition::on_btnSave_clicked()
{
    if (!peutModifier()) return;
    Competition c = lireFormulaire();
    const QString erreur = c.verifier();
    if (!erreur.isEmpty()) {
        ui->aErr->setText(erreur);
        ui->aErr->show();
        return;
    }
    ui->aErr->hide();

    const bool ajout = (idEnModification == 0);
    const bool ok = ajout ? c.ajouter() : c.modifier();   // CRUD dans la base Oracle
    if (!ok) {
        QMessageBox::critical(this, ajout ? "Ajout" : "Modification",
                              (ajout ? "La compétition n'a pas été ajoutée :\n"
                                     : "La compétition n'a pas été modifiée :\n")
                                  + Competition::derniereErreur());
        return;
    }

    // informations de l'interface (pas dans la table COMPETITION)
    QStringList noms;
    for (const QString &s : ui->aSponsors->text().split(',', Qt::SkipEmptyParts))
        if (!s.simplified().isEmpty()) noms << s.simplified();
    infos[c.getId()] = {c.getMode() == "En ligne" ? QString() : ui->aSalle->currentText().simplified(),
                        ui->aJury->text().simplified(),
                        ui->aDescription->toPlainText().trimmed(), noms.join(", ")};
    enregistrerInfos();

    QMessageBox::information(this, ajout ? "Ajout" : "Modification",
                             QString("Compétition « %1 » %2.").arg(c.getNom(), ajout ? "ajoutée" : "modifiée"));
    idSelectionne = c.getId();
    on_btnClear_clicked();
    if (ajout) reinitialiserFiltres();   // la nouvelle compétition doit être visible
    afficher();
    if (ajout) {   // aller à la page du nouvel élément
        for (int i = 0; i < filtree.size(); ++i)
            if (filtree.at(i).getId() == idSelectionne) page = i / PAR_PAGE;
        remplirTableau();
    }
}

void Gcompetition::on_btnClear_clicked()
{
    idEnModification = 0;
    ui->lblAdd->setText("Ajouter une compétition");
    ui->btnSave->setText("Enregistrer");
    ui->aId->setText("Auto");
    ui->aNom->clear();
    ui->aTheme->setCurrentIndex(0);
    ui->aDebut->setDate(QDate::currentDate());
    ui->aFin->setDate(QDate::currentDate().addDays(2));
    ui->aMode->setCurrentIndex(0);
    ui->aMax->setValue(5);
    ui->aStatut->setCurrentIndex(0);
    ui->aSalle->setCurrentIndex(-1);
    ui->aJury->clear();
    ui->aDescription->clear();
    ui->aSponsors->clear();
    ui->aErr->hide();
}

void Gcompetition::chargerFormulaire(int id)
{
    const int i = indexDe(id);
    if (i < 0 || !peutModifier()) return;
    const Competition &c = liste.at(i);
    const QStringList x = infos.value(id, VIDE);

    idEnModification = id;
    idSelectionne = id;
    ui->lblAdd->setText("Modifier la compétition");
    ui->btnSave->setText("Modifier");
    ui->aId->setText(QString::number(id));
    ui->aNom->setText(c.getNom());
    ui->aTheme->setCurrentText(c.getTheme());
    ui->aDebut->setDate(c.getDateDebut());
    ui->aFin->setDate(c.getDateFin());
    ui->aMode->setCurrentText(c.getMode());
    ui->aMax->setValue(c.getNbMaxMembres());
    ui->aStatut->setCurrentText(c.getStatut());
    ui->aSalle->setCurrentText(x.value(0));
    ui->aJury->setText(x.value(1));
    ui->aDescription->setPlainText(x.value(2));
    ui->aSponsors->setText(x.value(3));
    ui->aErr->hide();
    afficherDetails();
    ui->rightScroll->ensureWidgetVisible(ui->cardAdd);
    ui->aNom->setFocus();
}

// =====================================================================
//  SUPPRIMER
// =====================================================================
void Gcompetition::supprimer(int id)
{
    const int i = indexDe(id);
    if (i < 0 || !peutSupprimer()) return;
    const auto rep = QMessageBox::question(this, "Supprimer",
                                           QString("Supprimer la compétition « %1 » ?").arg(liste.at(i).getNom()));
    if (rep != QMessageBox::Yes) return;
    if (!Competition::supprimer(id)) {   // CRUD dans la base Oracle
        QMessageBox::critical(this, "Supprimer", "La compétition n'a pas été supprimée :\n"
                                                     + Competition::derniereErreur());
        return;
    }
    infos.remove(id);
    enregistrerInfos();
    if (idEnModification == id) on_btnClear_clicked();
    afficher();
}

// =====================================================================
//  Informations de l'interface (salle, jury, description, sponsors)
// =====================================================================
static QString cheminInfos()
{
    return QCoreApplication::applicationDirPath() + "/competitions_extra.json";
}

void Gcompetition::chargerInfos()
{
    infos.clear();
    QFile f(cheminInfos());
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject tout = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = tout.begin(); it != tout.end(); ++it) {
        const QJsonObject o = it.value().toObject();
        infos[it.key().toInt()] = {o.value("salle").toString(), o.value("jury").toString(),
                                   o.value("description").toString(), o.value("sponsors").toString()};
    }
}

void Gcompetition::enregistrerInfos()
{
    QJsonObject tout;
    for (auto it = infos.cbegin(); it != infos.cend(); ++it) {
        QJsonObject o;
        o["salle"] = it.value().value(0);
        o["jury"] = it.value().value(1);
        o["description"] = it.value().value(2);
        o["sponsors"] = it.value().value(3);
        tout[QString::number(it.key())] = o;
    }
    QFile f(cheminInfos());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(tout).toJson());
}

// =====================================================================
//  Affichage seulement : détails, chiffres, statistiques
// =====================================================================
void Gcompetition::afficherDetails()
{
    const int i = indexDe(idSelectionne);
    if (i < 0) {
        ui->dIco->setText("🏆");
        ui->dNom->setText("Aucune sélection");
        ui->dId->clear();
        ui->dBadge->clear();
        ui->dBadge->setStyleSheet("");
        for (QLabel *v : {ui->dTheme, ui->dDebut, ui->dFin, ui->dMode, ui->dMax, ui->dSalle,
                          ui->dJury, ui->dDescription, ui->dSponsors})
            v->setText("-");
        return;
    }
    const Competition &c = liste.at(i);
    const QStringList x = infos.value(c.getId(), VIDE);
    QLocale fr(QLocale::French);
    ui->dIco->setText(themeIcon(c.getTheme()));
    ui->dNom->setText(c.getNom());
    ui->dId->setText("ID : " + QString::number(c.getId()));
    ui->dBadge->setText(c.getStatut());
    ui->dBadge->setStyleSheet(QString("background:%1;color:%2;border-radius:10px;padding:2px 10px;"
                                      "font-size:11px;").arg(statutBg(c.getStatut()), statutFg(c.getStatut())));
    ui->dTheme->setText(c.getTheme());
    ui->dDebut->setText(fr.toString(c.getDateDebut(), "d MMMM yyyy"));
    ui->dFin->setText(fr.toString(c.getDateFin(), "d MMMM yyyy"));
    ui->dMode->setText(modeIcon(c.getMode()) + "  " + c.getMode());
    ui->dMax->setText(QString::number(c.getNbMaxMembres()));
    ui->dSalle->setText(c.getMode() == "En ligne" ? QString("En ligne (pas de salle)") : ouTiret(x.value(0)));
    ui->dJury->setText(ouTiret(x.value(1)));
    ui->dDescription->setText(ouTiret(x.value(2)));
    ui->dSponsors->setText(ouTiret(x.value(3)));
}

void Gcompetition::on_btnDetails_clicked()
{
    const int i = indexDe(idSelectionne);
    if (i < 0) return;
    const Competition &c = liste.at(i);
    const QStringList x = infos.value(c.getId(), VIDE);
    QMessageBox::information(this, c.getNom(),
        QString("ID : %1\nThème : %2\nDébut : %3\nFin : %4\nMode : %5\nMembres max : %6\nStatut : %7")
                .arg(c.getId()).arg(c.getTheme(), c.getDateDebut().toString("dd/MM/yyyy"),
                                    c.getDateFin().toString("dd/MM/yyyy"), c.getMode())
                .arg(c.getNbMaxMembres()).arg(c.getStatut())
            + QString("\nSalle : %1\nJury : %2\nSponsors : %3\n\n%4")
                  .arg(ouTiret(x.value(0)), ouTiret(x.value(1)), ouTiret(x.value(3)), x.value(2)));
}

void Gcompetition::mettreAJourStatistiques()
{
    const int total = int(liste.size());
    int equipes = 0;
    QMap<QString, int> parStatut, parTheme;
    QDate premier, dernier;
    for (const Competition &c : liste) {
        equipes += c.getNbMaxMembres();
        parStatut[c.getStatut()]++;
        parTheme[c.getTheme()]++;
        const QDate d = c.getDateDebut();
        if (!d.isValid()) continue;
        if (!premier.isValid() || d < premier) premier = d;
        if (!dernier.isValid() || d > dernier) dernier = d;
    }
    ui->kpiTotal->setText(QString::number(total));
    ui->kpiEquipes->setText(QString::number(equipes));
    ui->kpiPart->setText(QString::number(equipes * 4));   // estimation : 4 membres par équipe
    ui->kpiCours->setText(QString::number(parStatut.value("En cours")));

    // courbes : une valeur par mois de début
    mois.clear();
    equipesParMois.clear();
    participantsParMois.clear();
    if (premier.isValid()) {
        QLocale fr(QLocale::French);
        for (QDate m(premier.year(), premier.month(), 1); m <= dernier; m = m.addMonths(1)) {
            int n = 0;
            for (const Competition &c : liste)
                if (c.getDateDebut().year() == m.year() && c.getDateDebut().month() == m.month())
                    n += c.getNbMaxMembres();
            mois << fr.toString(m, "MMM yy");
            equipesParMois << n;
            participantsParMois << n * 4;
        }
    }
    ui->statsChart->setText(QString());
    ui->statsChart->update();

    const int faites = parStatut.value("En cours") + parStatut.value("Terminée");
    ui->lblTaux->setText(QString("<b>Taux de participation : %1 %</b>  (compétitions en cours ou terminées)")
                             .arg(total ? qRound(100.0 * faites / total) : 0));
    ui->legCours->setText(QString::number(parStatut.value("En cours")));
    ui->legPlanifiee->setText(QString::number(parStatut.value("Planifiée")));
    ui->legTerminee->setText(QString::number(parStatut.value("Terminée")));

    // une barre par thème, du plus fréquent au moins fréquent
    QList<QPair<int, QString>> themes;
    for (auto it = parTheme.cbegin(); it != parTheme.cend(); ++it) themes.append({it.value(), it.key()});
    std::sort(themes.begin(), themes.end(), [](const QPair<int, QString> &a, const QPair<int, QString> &b) {
        return a.first != b.first ? a.first > b.first : a.second < b.second;
    });
    while (QLayoutItem *it = ui->themesLayout->takeAt(0)) {
        if (QWidget *w = it->widget()) { w->hide(); w->deleteLater(); }
        delete it;
    }
    for (const auto &t : themes) {
        QWidget *ligne = new QWidget;
        QHBoxLayout *l = new QHBoxLayout(ligne);
        l->setContentsMargins(0, 0, 0, 0);
        QLabel *nom = new QLabel(themeIcon(t.second) + "  " + t.second);
        nom->setMinimumWidth(150);
        QProgressBar *barre = new QProgressBar;
        barre->setObjectName("themeBar");
        barre->setRange(0, qMax(1, total));
        barre->setValue(t.first);
        barre->setTextVisible(false);
        barre->setFixedHeight(8);
        QLabel *nb = new QLabel(QString::number(t.first));
        nb->setMinimumWidth(20);
        nb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->addWidget(nom);
        l->addWidget(barre, 1);
        l->addWidget(nb);
        ui->themesLayout->addWidget(ligne);
    }
}

// deux courbes (équipes, participants estimés), un point par mois
void Gcompetition::dessinerCourbes(QWidget *zone)
{
    QPainter p(zone);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor grille("#E5EAF3"), axe("#94A3B8"), gris("#64748B");
    const QStringList noms = {"Équipes", "Participants (estimés)"};
    const QList<QColor> couleurs = {QColor("#2563EB"), QColor("#FF6B00")};
    const QVector<QVector<double>> series = {equipesParMois, participantsParMois};

    // légende
    QFont petit = zone->font();
    petit.setPixelSize(11);
    p.setFont(petit);
    qreal lx = 4;
    for (int s = 0; s < 2; ++s) {
        p.setPen(QPen(couleurs[s], 2.4));
        p.drawLine(QPointF(lx, 9), QPointF(lx + 14, 9));
        p.setPen(gris);
        const qreal w = p.fontMetrics().horizontalAdvance(noms[s]);
        p.drawText(QRectF(lx + 18, 0, w + 2, 18), Qt::AlignLeft | Qt::AlignVCenter, noms[s]);
        lx += 18 + w + 14;
    }

    const QRectF zoneCourbe(34, 30, zone->width() - 44, zone->height() - 52);
    if (mois.isEmpty()) {
        p.setPen(gris);
        p.drawText(zoneCourbe, Qt::AlignCenter, "Aucune donnée");
        return;
    }

    // échelle « ronde » : 1, 2, 5, 10, 20, 50…
    double maxi = 1;
    for (const auto &s : series)
        for (double v : s) maxi = std::max(maxi, v);
    const int graduations = 4;
    const double brut = std::max(1.0, maxi / graduations);
    const double puissance = std::pow(10.0, std::floor(std::log10(brut)));
    const double n = brut / puissance;
    const double pas = (n <= 1 ? 1 : n <= 2 ? 2 : n <= 5 ? 5 : 10) * puissance;
    const double yMax = pas * graduations;
    const int nb = int(mois.size());
    auto Y = [&](double v) { return zoneCourbe.bottom() - zoneCourbe.height() * v / yMax; };
    auto X = [&](int i) {
        return nb <= 1 ? zoneCourbe.center().x() : zoneCourbe.left() + zoneCourbe.width() * i / (nb - 1);
    };

    for (int t = 0; t <= graduations; ++t) {
        const double v = pas * t;
        p.setPen(QPen(t == 0 ? axe : grille, t == 0 ? 1.0 : 0.8));
        p.drawLine(QPointF(zoneCourbe.left(), Y(v)), QPointF(zoneCourbe.right(), Y(v)));
        p.setPen(gris);
        p.drawText(QRectF(0, Y(v) - 8, 28, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(v));
    }
    const int chaque = std::max(1, (nb + 4) / 5);
    for (int i = 0; i < nb; ++i) {
        if (i % chaque != 0 && i != nb - 1) continue;
        p.drawText(QRectF(X(i) - 32, zoneCourbe.bottom() + 4, 64, 16), Qt::AlignCenter, mois[i]);
    }

    for (int s = 0; s < 2; ++s) {
        QPainterPath chemin;
        for (int i = 0; i < nb; ++i) {
            const QPointF pt(X(i), Y(series[s].value(i)));
            if (i == 0) chemin.moveTo(pt); else chemin.lineTo(pt);
        }
        p.setPen(QPen(couleurs[s], 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(chemin);
        for (int i = 0; i < nb; ++i) {
            const QPointF pt(X(i), Y(series[s].value(i)));
            p.setPen(QPen(Qt::white, 1.5));
            p.setBrush(couleurs[s]);
            p.drawEllipse(pt, 3.6, 3.6);
            if (nb <= 8) {
                p.setPen(couleurs[s]);
                p.drawText(QRectF(pt.x() - 20, pt.y() - 19, 40, 14), Qt::AlignCenter,
                           QString::number(series[s].value(i)));
            }
        }
    }
}
