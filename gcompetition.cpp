// Gcompetition : fenêtre du module Gestion des compétitions (interface).
// Les widgets sont dans gcompetition.ui (Qt Designer) et les couleurs dans style.qss
// (chargé dans main.cpp) ; ce fichier contient la logique.
// Les compétitions sont enregistrées dans la base Oracle (classe Competition, crudcompetition.cpp).

#include "gcompetition.h"
#include "ui_gcompetition.h"
#include "animations.h"
#include "calendarview.h"
#include "pdfexport.h"
#include "statschart.h"
#include "planning.h"
#include "xlsxwriter.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QComboBox>
#include <QDateEdit>
#include <QDesktopServices>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QAction>
#include <QEvent>
#include <QFileDialog>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPixmap>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSet>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

// ---------------------------------------------------------------- Données
// Les compétitions sont lues et enregistrées dans la base Oracle par la classe Competition
// (competition.h/.cpp). Salle, jury, description et sponsors ne sont pas des colonnes de la
// table COMPETITION : l'application les garde dans competitions_extra.json, avec l'ID.

static int g_idEmploye = 0;   // employé du rôle connecté : responsable des nouvelles compétitions

static QString extraPath() {
    return QCoreApplication::applicationDirPath() + "/competitions_extra.json";
}

static void saveExtras(const QVector<Competition> &data) {
    QJsonObject all;
    for (const Competition &c : data) {
        QJsonObject o;
        o["salle"] = c.getSalle();
        o["jury"] = c.getJury();
        o["description"] = c.getDescription();
        o["sponsors"] = c.getSponsors();
        all[c.getId()] = o;
    }
    QFile f(extraPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(all).toJson());
}

// toutes les compétitions de la base + leurs informations d'interface
static QVector<Competition> loadData() {
    QJsonObject extras;
    QFile f(extraPath());
    if (f.open(QIODevice::ReadOnly)) extras = QJsonDocument::fromJson(f.readAll()).object();

    QVector<Competition> out = Competition::lister();
    for (Competition &c : out) {
        const QJsonObject x = extras.value(c.getId()).toObject();
        c.setSalle(c.getMode() == "En ligne" ? QString() : x.value("salle").toString());
        c.setJury(x.value("jury").toString());
        c.setDescription(x.value("description").toString());
        c.setSponsors(x.value("sponsors").toString());
    }
    return out;
}

// ---------------------------------------------------------------- Vérification du planning
// Appelée juste avant d'enregistrer une compétition (ajout ou modification).
// Conflit de salle ou de jury -> refus, avec une date libre proposée.
// Simple avertissement sur les dates -> l'utilisateur choisit d'enregistrer ou non.
static bool confirmPlanning(QWidget *parent, const Competition &c, const QVector<Competition> &all) {
    const QVector<Conflict> conflicts = detectConflicts(c, all);
    if (conflicts.isEmpty()) return true;

    QStringList blocking, warnings;
    for (const Conflict &k : conflicts) (k.blocking ? blocking : warnings) << "•  " + k.message;

    if (!blocking.isEmpty()) {
        QString text = "La compétition ne peut pas être validée :\n\n" + blocking.join("\n");
        if (!warnings.isEmpty()) text += "\n\nÀ vérifier aussi :\n" + warnings.join("\n");
        const QDate free = nextFreeStart(c, all);
        if (free.isValid()) {
            const qint64 length = QDate::fromString(c.getDebut(), DATE_FMT).daysTo(QDate::fromString(c.getFin(), DATE_FMT));
            text += QString("\n\nSuggestion : avec la même salle et le même jury, la période du %1 au %2 est libre.")
                        .arg(free.toString(DATE_FMT), free.addDays(length).toString(DATE_FMT));
        }
        QMessageBox::critical(parent, "Conflit de planning", text);
        return false;
    }
    const auto answer = QMessageBox::warning(
        parent, "Vérification du planning",
        "Points à vérifier avant de valider :\n\n" + warnings.join("\n") + "\n\nEnregistrer quand même ?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

// ---------------------------------------------------------------- Fenêtre « Modifier »
// Seule la modification (bouton ✏️ du tableau) ouvre une fenêtre ; l'ajout se fait
// dans la colonne droite de la fenêtre principale.
class CompetitionDialog : public QDialog {
public:
    CompetitionDialog(QWidget *parent, const Competition &comp, const QVector<Competition> &all)
        : QDialog(parent), compId(comp.getId()) {
        setWindowTitle("Modifier la compétition");
        setMinimumWidth(420);

        nom = new QLineEdit(comp.getNom());
        theme = new QComboBox; theme->addItems(THEMES); theme->setCurrentText(comp.getTheme());
        debut = new QDateEdit(QDate::fromString(comp.getDebut(), DATE_FMT)); debut->setCalendarPopup(true);
        fin = new QDateEdit(QDate::fromString(comp.getFin(), DATE_FMT)); fin->setCalendarPopup(true);
        debut->setDisplayFormat(DATE_FMT); fin->setDisplayFormat(DATE_FMT);
        mode = new QComboBox; mode->addItems(MODES); mode->setCurrentText(comp.getMode());
        max = new QSpinBox; max->setRange(1, 50); max->setValue(comp.getMax());
        statut = new QComboBox; statut->addItems(STATUTS); statut->setCurrentText(comp.getStatut());
        salle = new QComboBox; salle->setEditable(true); salle->addItems(SALLES); salle->setCurrentText(comp.getSalle());
        jury = new QLineEdit(comp.getJury()); jury->setPlaceholderText("Noms séparés par des virgules");
        description = new QPlainTextEdit(comp.getDescription());
        description->setPlaceholderText("Objectif, règles, public visé…");
        description->setMaximumHeight(80);
        sponsors = new QLineEdit(comp.getSponsors()); sponsors->setPlaceholderText("Sponsors séparés par des virgules");
        // Une compétition en ligne n'occupe pas de salle.
        auto syncSalle = [this]() { salle->setEnabled(mode->currentText() != "En ligne"); };
        connect(mode, &QComboBox::currentTextChanged, this, syncSalle);
        syncSalle();

        QFormLayout *form = new QFormLayout;
        form->addRow("ID", new QLabel(compId));
        form->addRow("Nom *", nom);
        form->addRow("Thème", theme);
        form->addRow("Date début", debut);
        form->addRow("Date fin", fin);
        form->addRow("Mode", mode);
        form->addRow("Membres max", max);
        form->addRow("Salle", salle);
        form->addRow("Jury", jury);
        form->addRow("Statut", statut);
        form->addRow("Description", description);
        form->addRow("Sponsors", sponsors);

        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        bb->button(QDialogButtonBox::Ok)->setText("Enregistrer");
        bb->button(QDialogButtonBox::Cancel)->setText("Annuler");
        connect(bb, &QDialogButtonBox::accepted, this, [this, all]() {
            if (nom->text().trimmed().isEmpty()) {
                QMessageBox::warning(this, "Champ requis", "Le nom de la compétition est obligatoire.");
                return;
            }
            if (fin->date() < debut->date()) {
                QMessageBox::warning(this, "Dates invalides",
                                     "La date de fin doit être après la date de début.");
                return;
            }
            if (!confirmPlanning(this, result(), all)) return;  // conflits de planning
            accept();
        });
        connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *lay = new QVBoxLayout(this);
        lay->addLayout(form);
        lay->addWidget(bb);
    }

    Competition result() const {
        Competition c;
        c.setId(compId);
        c.setNom(nom->text().trimmed());
        c.setTheme(theme->currentText());
        c.setDebut(debut->date().toString(DATE_FMT));
        c.setFin(fin->date().toString(DATE_FMT));
        c.setMode(mode->currentText());
        c.setMax(max->value());
        c.setStatut(statut->currentText());
        c.setSalle(c.getMode() == "En ligne" ? QString() : salle->currentText().simplified());
        c.setJury(jury->text().simplified());
        c.setDescription(description->toPlainText().trimmed());
        c.setSponsors(splitNames(sponsors->text()).join(", "));
        return c;
    }

private:
    QString compId;
    QLineEdit *nom, *jury, *sponsors;
    QPlainTextEdit *description;
    QComboBox *theme, *mode, *statut, *salle;
    QDateEdit *debut, *fin;
    QSpinBox *max;
};

// ---------------------------------------------------------------- Fenêtre principale
Gcompetition::Gcompetition(QWidget *parent) : QMainWindow(parent), ui(new Ui::Gcompetition) {
    ui->setupUi(this);  // crée tous les widgets dessinés dans gcompetition.ui

    // SWHackTime : une page par module. Page 0 = module Compétitions (PComp).
    ui->SWHackTime->setCurrentIndex(0);
    ui->SWCompet->setCurrentIndex(0);

    // Le nom de l'utilisateur en haut à droite sert aussi à se déconnecter.
    ui->admin->setCursor(Qt::PointingHandCursor);
    ui->admin->installEventFilter(this);

    // Ne jamais dépasser l'écran : sinon le bas de la fenêtre est inaccessible.
    QSize winSize(1536, 960);
    if (QScreen *sc = QGuiApplication::primaryScreen())
        winSize = winSize.boundedTo(sc->availableGeometry().size() - QSize(20, 50));
    resize(winSize);

    // La colonne droite laisse voir le fond de la fenêtre.
    ui->rightScroll->viewport()->setAutoFillBackground(false);
    ui->rightCol->setAutoFillBackground(false);

    data = loadData();
    if (!data.isEmpty()) selectedId = data.first().getId();

    makeBlink(ui->logo, this)->start();  // le logo du menu de gauche clignote aussi

    setupNavigation();
    setupExports();
    setupTable();
    setupFilters();
    setupAddForm();

    connect(ui->btnDetails, &QPushButton::clicked, this, [this]() { showDetails(); });
    connect(ui->btnTeams, &QPushButton::clicked, this, [this]() { notImplemented("Équipes"); });
    connect(ui->btnViewJury, &QPushButton::clicked, this, [this]() { notImplemented("Jury"); });
    connect(ui->btnReports, &QPushButton::clicked, this, [this]() { notImplemented("Rapports"); });

    refresh();
}

Gcompetition::~Gcompetition() {
    delete ui;
}

// Rôle choisi sur la page de connexion (Administrateur, Organisateur ou Jury).
void Gcompetition::setRole(const QString &r) {
    role = r;
    g_idEmploye = Competition::idEmployeParRole(r);   // #id_employe des compétitions ajoutées
    applyRole();
}

// Droits du rôle connecté : formulaire d'ajout, boutons ✏️ / 🗑️, nom affiché en haut.
void Gcompetition::applyRole() {
    ui->admin->setText(QString("👤  <b>%1</b>  ▾").arg(role));
    ui->admin->setToolTip("Connecté en tant que " + role + " — cliquer pour se déconnecter");
    ui->cardAdd->setVisible(canEdit());
    refresh();
}

bool Gcompetition::canEdit() const { return role == "Administrateur" || role == "Organisateur"; }
bool Gcompetition::canDelete() const { return role == "Administrateur"; }

// Clic sur le nom en haut à droite : retour à la page de connexion (voir main.cpp).
void Gcompetition::logout() {
    if (QMessageBox::question(this, "Déconnexion", "Se déconnecter de HackTime ?") != QMessageBox::Yes) return;
    role.clear();
    emit loggedOut();
}

// Clic sur le nom de l'utilisateur -> déconnexion ; molette sur un champ du formulaire
// d'ajout -> fait défiler la colonne au lieu de changer la valeur.
bool Gcompetition::eventFilter(QObject *obj, QEvent *ev) {
    if (obj == ui->admin) {
        if (ev->type() == QEvent::MouseButtonPress) {
            logout();
            return true;
        }
        return QMainWindow::eventFilter(obj, ev);
    }
    if (ev->type() == QEvent::Wheel) {
        ev->ignore();   // non accepté -> l'événement remonte jusqu'à la zone de défilement
        return true;
    }
    return QMainWindow::eventFilter(obj, ev);
}

// ---------------------------------------------------------------- Mise en place
void Gcompetition::setupNavigation() {
    // Seul le module « Compétitions » existe ; les autres boutons affichent un message.
    const QList<QPair<QPushButton *, QString>> others = {
        {ui->btnDashboard, "Tableau de bord"}, {ui->btnEquipes, "Équipes"},
        {ui->btnParticipants, "Participants"}, {ui->btnAtelier, "Atelier"},
        {ui->btnEmployes, "Employés"}, {ui->btnParametres, "Paramètres"}};
    for (const auto &o : others) {
        const QString name = o.second;
        connect(o.first, &QPushButton::clicked, this, [this, name]() {
            notImplemented(name);
            ui->btnCompetitions->setChecked(true);  // on reste sur « Compétitions »
        });
    }
}

// Boutons « Exporter PDF » et « Exporter Excel » de l'en-tête.
void Gcompetition::setupExports() {
    connect(ui->btnExportPdf, &QPushButton::clicked, this, [this]() { exportPdf(); });
    connect(ui->btnExportExcel, &QPushButton::clicked, this, [this]() { exportExcel(); });
    connect(ui->btnConflicts, &QPushButton::clicked, this, [this]() { showAllConflicts(); });
    connect(ui->btnCalendar, &QPushButton::clicked, this, [this]() { openCalendar(); });
}

// Export PDF : planning des sponsors (frise dans le temps, puis tableau par sponsor),
// en A4 paysage. Suit les filtres, la recherche et le tri.
void Gcompetition::exportPdf() {
    if (filtered.isEmpty()) {
        QMessageBox::information(this, "Exporter PDF", "Aucune compétition à exporter avec les filtres actuels.");
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, "Enregistrer le PDF",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Sponsors_HackTime.pdf",
        "Document PDF (*.pdf)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    writer.setResolution(72);  // 1 unité = 1 point : la page fait 842 x 595
    writer.setTitle("HackTime : planning des sponsors");
    writer.setCreator("HackTime");

    QPainter p;
    if (!p.begin(&writer)) {
        QMessageBox::warning(this, "Exporter PDF",
                             "Impossible d'écrire le fichier. Il est peut-être ouvert dans un autre programme.");
        return;
    }
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    pdf::drawReport(writer, p, filtered, QImage(":/images/logo_hacktime_clair.png"));
    p.end();
    showExported("Exporter PDF", path);
}

// Liste des compétitions affichées (filtres et recherche compris) dans un vrai classeur .xlsx.
void Gcompetition::exportExcel() {
    if (filtered.isEmpty()) {
        QMessageBox::information(this, "Exporter Excel", "Aucune compétition à exporter avec les filtres actuels.");
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, "Enregistrer le fichier Excel",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Competitions_HackTime.xlsx",
        "Classeur Excel (*.xlsx)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".xlsx", Qt::CaseInsensitive)) path += ".xlsx";

    auto t = [](const QString &s) { return xlsx::text(s.toStdString()); };
    std::vector<xlsx::Row> rows;
    rows.push_back({t("ID"), t("Nom"), t("Thème"), t("Date début"), t("Date fin"), t("Mode"),
                    t("Membres max"), t("Salle"), t("Jury"), t("Statut"), t("Planning"), t("Sponsors"), t("Description")});
    for (const Competition &c : filtered) {
        int blocking = 0;
        for (const Conflict &k : detectConflicts(c, data))
            if (k.blocking) ++blocking;
        rows.push_back({t(c.getId()), t(c.getNom()), t(c.getTheme()), t(c.getDebut()), t(c.getFin()), t(c.getMode()),
                        xlsx::number(c.getMax()), t(c.getSalle()), t(c.getJury()), t(c.getStatut()),
                        t(blocking ? QString("%1 conflit(s)").arg(blocking) : QString("OK")), t(c.getSponsors()),
                        t(c.getDescription())});
    }
    const std::string bytes = xlsx::build("Compétitions", rows, {10, 34, 24, 13, 13, 13, 13, 18, 40, 13, 14, 34, 60});

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "Exporter Excel",
                             "Impossible d'écrire le fichier. Il est peut-être ouvert dans Excel : fermez-le et réessayez.");
        return;
    }
    f.write(bytes.data(), qint64(bytes.size()));
    f.close();
    showExported("Exporter Excel", path);
}

// Message de fin d'export, avec un bouton pour ouvrir le fichier.
void Gcompetition::showExported(const QString &title, const QString &path) {
    QMessageBox box(QMessageBox::Information, title,
                    "Fichier enregistré :\n" + QDir::toNativeSeparators(path), QMessageBox::NoButton, this);
    QPushButton *open = box.addButton("Ouvrir", QMessageBox::AcceptRole);
    box.addButton("Fermer", QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == open) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void Gcompetition::setupTable() {
    QTableWidget *table = ui->table;
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    const int widths[] = {70, 0, 170, 100, 100, 110, 100, 100, 120};
    for (int i = 0; i < table->columnCount() && i < 9; ++i)
        if (widths[i]) table->setColumnWidth(i, widths[i]);
    connect(table, &QTableWidget::itemSelectionChanged, this, [this]() { onSelect(); });
}

void Gcompetition::setupFilters() {
    ui->fStatut->addItem("Tous"); ui->fStatut->addItems(STATUTS);
    ui->fTheme->addItem("Tous"); ui->fTheme->addItems(THEMES);
    ui->fSort->addItems({"Date début ↑", "Date début ↓", "Date fin ↑", "Date fin ↓", "ID"});

    connect(ui->topSearch, &QLineEdit::textChanged, this,
            [this](const QString &t) { ui->search->setText(t); });
    connect(ui->search, &QLineEdit::textChanged, this, [this]() { onFilter(); });
    for (QComboBox *c : {ui->fStatut, ui->fTheme, ui->fDate, ui->fSort})
        connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](int) { onFilter(); });
}

void Gcompetition::setupAddForm() {
    ui->aTheme->addItems(THEMES);
    ui->aMode->addItems(MODES);
    ui->aStatut->addItems(STATUTS);
    ui->aSalle->addItems(SALLES);
    ui->aConflict->setTextFormat(Qt::RichText);
    ui->dId->setTextFormat(Qt::RichText);

    const QList<QWidget *> fields = {ui->aTheme, ui->aDebut, ui->aFin,
                                     ui->aMode, ui->aMax, ui->aSalle, ui->aStatut};
    for (QWidget *f : fields) {
        f->setFocusPolicy(Qt::StrongFocus);  // la molette ne donne plus le focus
        f->installEventFilter(this);         // ... et ne modifie plus la valeur
    }

    connect(ui->btnSave, &QPushButton::clicked, this, [this]() { submitAddForm(); });
    connect(ui->btnClear, &QPushButton::clicked, this, [this]() { resetAddForm(); });
    connect(ui->aNom, &QLineEdit::returnPressed, this, [this]() { submitAddForm(); });

    // Détection des conflits en direct, pendant la saisie.
    connect(ui->aDebut, &QDateEdit::dateChanged, this, [this]() { updateAddConflicts(); });
    connect(ui->aFin, &QDateEdit::dateChanged, this, [this]() { updateAddConflicts(); });
    connect(ui->aSalle, &QComboBox::currentTextChanged, this, [this]() { updateAddConflicts(); });
    connect(ui->aJury, &QLineEdit::textChanged, this, [this]() { updateAddConflicts(); });
    connect(ui->aStatut, &QComboBox::currentTextChanged, this, [this]() { updateAddConflicts(); });
    connect(ui->aMode, &QComboBox::currentTextChanged, this, [this](const QString &m) {
        ui->aSalle->setEnabled(m != "En ligne");  // une compétition en ligne n'occupe pas de salle
        updateAddConflicts();
    });
    resetAddForm();
}

// ---------------------------------------------------------------- Utilitaires
QString Gcompetition::nextId() const {
    return "Auto";  // l'ID est donné par Oracle (séquence + trigger) à l'enregistrement
}

int Gcompetition::indexOf(const QString &id) const {
    for (int i = 0; i < data.size(); ++i)
        if (data[i].getId() == id) return i;
    return -1;
}

void Gcompetition::notImplemented(const QString &module) {
    QMessageBox::information(this, module,
                             QString("Le module « %1 » n'est pas encore implémenté.").arg(module));
}

// ---------------------------------------------------------------- Formulaire d'ajout
// Remet le formulaire (colonne droite, sous les détails) à ses valeurs par défaut.
void Gcompetition::resetAddForm() {
    ui->aId->setText(nextId());
    ui->aNom->clear();
    ui->aTheme->setCurrentIndex(0);
    ui->aDebut->setDate(QDate::currentDate());
    ui->aFin->setDate(QDate::currentDate().addDays(2));
    ui->aMode->setCurrentIndex(0);
    ui->aMax->setValue(5);
    ui->aStatut->setCurrentIndex(0);
    ui->aSalle->setCurrentIndex(-1);
    ui->aSalle->setEditText(QString());
    ui->aJury->clear();
    ui->aDescription->clear();
    ui->aSponsors->clear();
    ui->aErr->hide();
    updateAddConflicts();
}

// Compétition telle qu'elle est saisie dans le formulaire d'ajout.
Competition Gcompetition::formComp() const {
    Competition c;
    c.setId(nextId());
    c.setNom(ui->aNom->text().trimmed());
    c.setTheme(ui->aTheme->currentText());
    c.setDebut(ui->aDebut->date().toString(DATE_FMT));
    c.setFin(ui->aFin->date().toString(DATE_FMT));
    c.setMode(ui->aMode->currentText());
    c.setMax(ui->aMax->value());
    c.setStatut(ui->aStatut->currentText());
    c.setSalle(c.getMode() == "En ligne" ? QString() : ui->aSalle->currentText().simplified());
    c.setJury(ui->aJury->text().simplified());
    c.setDescription(ui->aDescription->toPlainText().trimmed());
    c.setSponsors(splitNames(ui->aSponsors->text()).join(", "));
    return c;
}

// Bandeau sous le formulaire : conflits de planning détectés pendant la saisie.
void Gcompetition::updateAddConflicts() {
    const QVector<Conflict> conflicts = detectConflicts(formComp(), data);
    if (conflicts.isEmpty()) {
        ui->aConflict->setText("✔  Aucun conflit de planning (dates, salle, jury).");
        ui->aConflict->setStyleSheet("color:#15803D; background:#DCFCE7; border-radius:8px; padding:8px 10px;"
                                     "font-size:12px;");
        return;
    }
    QStringList lines;
    for (const Conflict &k : conflicts)
        lines << QString("%1  %2").arg(k.blocking ? QString("⛔") : QString("⚠️"), k.message.toHtmlEscaped());
    const bool blocking = hasBlocking(conflicts);
    ui->aConflict->setText(lines.join("<br>"));
    ui->aConflict->setStyleSheet(blocking
        ? "color:#B91C1C; background:#FEE2E2; border-radius:8px; padding:8px 10px; font-size:12px;"
        : "color:#92400E; background:#FEF3C7; border-radius:8px; padding:8px 10px; font-size:12px;");
}

// Calendrier des événements : toutes les compétitions mois par mois, conflits en rouge.
// Il s'ouvre sur le mois de la compétition sélectionnée ; un clic sur une compétition la sélectionne.
void Gcompetition::openCalendar() {
    QSet<QString> conflicts;
    for (const Competition &c : data)
        if (hasBlocking(detectConflicts(c, data))) conflicts.insert(c.getId());
    const int i = indexOf(selectedId);
    const QDate start = i >= 0 ? QDate::fromString(data[i].getDebut(), DATE_FMT) : QDate::currentDate();

    CalendarDialog dlg(this, data, conflicts, start);
    if (dlg.exec() == QDialog::Accepted && !dlg.pickedId().isEmpty()) {
        selectedId = dlg.pickedId();
        updateDetails();
        selectRow(selectedId);  // surligne la ligne si elle est sur la page affichée
    }
}

// Bouton « Vérifier les conflits » : rapport sur tout le planning.
void Gcompetition::showAllConflicts() {
    QStringList blocking, warnings;
    QSet<QString> seen;  // un conflit entre A et B n'est affiché qu'une fois
    for (const Competition &c : data) {
        for (const Conflict &k : detectConflicts(c, data)) {
            if (k.blocking && !k.otherId.isEmpty()) {
                const QString key = QString::number(int(k.kind)) + "|" + qMin(c.getId(), k.otherId) + "|" + qMax(c.getId(), k.otherId);
                if (seen.contains(key)) continue;
                seen.insert(key);
            }
            (k.blocking ? blocking : warnings) << QString("•  %1 « %2 » : %3").arg(c.getId(), c.getNom(), k.message);
        }
    }
    if (blocking.isEmpty() && warnings.isEmpty()) {
        QMessageBox::information(this, "Vérification du planning",
                                 "Aucun conflit : dates, salles et jurys sont compatibles pour toutes les compétitions.");
        return;
    }
    QString text;
    if (!blocking.isEmpty())
        text += QString("%1 conflit(s) de salle ou de jury :\n\n").arg(blocking.size()) + blocking.join("\n");
    if (!warnings.isEmpty())
        text += QString(text.isEmpty() ? "" : "\n\n") +
                QString("%1 remarque(s) sur les dates :\n\n").arg(warnings.size()) + warnings.join("\n");
    QMessageBox box(blocking.isEmpty() ? QMessageBox::Warning : QMessageBox::Critical,
                    "Vérification du planning", text, QMessageBox::Ok, this);
    box.exec();
}

void Gcompetition::showAddError(const QString &msg) {
    ui->aErr->setText(msg);
    ui->aErr->show();
}

void Gcompetition::submitAddForm() {
    if (!canEdit()) return;
    if (ui->aNom->text().trimmed().isEmpty()) {
        showAddError("Le nom de la compétition est obligatoire.");
        ui->aNom->setFocus();
        return;
    }
    if (ui->aFin->date() < ui->aDebut->date()) {
        showAddError("La date de fin doit être après la date de début.");
        return;
    }
    Competition c = formComp();
    if (!confirmPlanning(this, c, data)) return;  // conflits de planning (dates, salle, jury)

    // CRUD : ajout dans la base Oracle
    if (c.getIdEmploye() <= 0) c.setIdEmploye(g_idEmploye);
    const QString err = c.verifier();
    if (!err.isEmpty()) {
        showAddError(err);
        return;
    }
    if (!c.ajouter()) {   // ajouter() donne aussi son ID à la compétition
        QMessageBox::critical(this, "Ajout", "La compétition n'a pas été ajoutée :\n" +
                                                 Competition::derniereErreur());
        return;
    }
    data.append(c);
    saveExtras(data);
    QMessageBox::information(this, "Ajout", QString("Compétition « %1 » ajoutée.").arg(c.getNom()));
    selectedId = c.getId();
    resetFilters();
    page = (int(data.size()) - 1) / PER_PAGE;  // aller à la page du nouvel élément
    resetAddForm();                            // prêt pour une nouvelle saisie
    refresh();
}

// ---------------------------------------------------------------- Actions
void Gcompetition::editCompetition(const QString &id) {
    if (!canEdit()) return;
    int i = indexOf(id);
    if (i < 0) return;
    CompetitionDialog dlg(this, data[i], data);
    if (dlg.exec() == QDialog::Accepted) {
        Competition c = dlg.result();
        c.setIdEmploye(data[i].getIdEmploye() > 0 ? data[i].getIdEmploye() : g_idEmploye);
        const QString err = c.verifier();          // CRUD : modification dans la base Oracle
        if (!err.isEmpty()) {
            QMessageBox::warning(this, "Modifier", err);
            return;
        }
        if (!c.modifier()) {
            QMessageBox::critical(this, "Modifier", "La compétition n'a pas été modifiée :\n" +
                                                        Competition::derniereErreur());
            return;
        }
        data[i] = c;
        saveExtras(data);
        refresh();
    }
}

void Gcompetition::deleteCompetition(const QString &id) {
    if (!canDelete()) return;
    int i = indexOf(id);
    if (i < 0) return;
    auto r = QMessageBox::question(this, "Supprimer",
                QString("Supprimer la compétition « %1 » ?").arg(data[i].getNom()));
    if (r == QMessageBox::Yes) {
        if (!Competition::supprimer(id.toInt())) {   // CRUD : suppression dans la base Oracle
            QMessageBox::critical(this, "Supprimer", "La compétition n'a pas été supprimée :\n" +
                                                         Competition::derniereErreur());
            return;
        }
        data.remove(i);
        saveExtras(data);
        if (selectedId == id) selectedId = data.isEmpty() ? QString() : data.first().getId();
        refresh();
    }
}

void Gcompetition::selectRow(const QString &id) {
    selectedId = id;
    updateDetails();
    for (int r = 0; r < ui->table->rowCount(); ++r) {
        if (ui->table->item(r, 0)->text() == id) {
            QSignalBlocker b(ui->table);
            ui->table->selectRow(r);
        }
    }
}

void Gcompetition::showDetails() {
    int i = indexOf(selectedId);
    if (i < 0) return;
    const Competition &c = data[i];
    QMessageBox::information(this, c.getNom(),
        QString("ID : %1\nThème : %2\nDébut : %3\nFin : %4\nMode : %5\nMembres max : %6\nStatut : %7")
            .arg(c.getId(), c.getTheme(), c.getDebut(), c.getFin(), c.getMode()).arg(c.getMax()).arg(c.getStatut())
            + QString("\nSalle : %1\nJury : %2\nSponsors : %3\n\n%4")
                  .arg(c.getSalle().isEmpty() ? QString("-") : c.getSalle(), c.getJury().isEmpty() ? QString("-") : c.getJury(),
                       c.getSponsors().isEmpty() ? QString("-") : c.getSponsors(), c.getDescription()));
}

// ---------------------------------------------------------------- Filtres
void Gcompetition::onSelect() {
    const auto rows = ui->table->selectionModel()->selectedRows();
    if (!rows.isEmpty()) {
        selectedId = ui->table->item(rows.first().row(), 0)->text();
        updateDetails();
    }
}

void Gcompetition::onFilter() {
    page = 0;
    refresh(false);
}

void Gcompetition::resetFilters() {
    QSignalBlocker b1(ui->search), b2(ui->fStatut), b3(ui->fTheme), b4(ui->fDate), b5(ui->topSearch);
    ui->search->clear(); ui->topSearch->clear();
    ui->fStatut->setCurrentIndex(0); ui->fTheme->setCurrentIndex(0); ui->fDate->setCurrentIndex(0);
}

QVector<Competition> Gcompetition::applyFilters() const {
    QString q = ui->search->text().trimmed().toLower();
    QString st = ui->fStatut->currentText(), th = ui->fTheme->currentText(),
            yr = ui->fDate->currentText();
    QVector<Competition> out;
    for (const Competition &c : data) {
        if (!q.isEmpty() && !c.getNom().toLower().contains(q) && !c.getId().toLower().contains(q)
            && !c.getTheme().toLower().contains(q) && !c.getDescription().toLower().contains(q)
            && !c.getSponsors().toLower().contains(q)) continue;
        if (st != "Tous" && c.getStatut() != st) continue;
        if (th != "Tous" && c.getTheme() != th) continue;
        if (!yr.isEmpty() && yr != "Toutes" && !c.getDebut().endsWith(yr)) continue;
        out.append(c);
    }

    // Tri choisi dans « Trier par » (date de début ou de fin, croissant ou décroissant).
    const int sort = ui->fSort->currentIndex();
    if (sort >= 0 && sort <= 3) {
        const bool byEnd = sort >= 2, descending = sort % 2 == 1;
        std::stable_sort(out.begin(), out.end(), [byEnd, descending](const Competition &a, const Competition &b) {
            const QDate da = QDate::fromString(byEnd ? a.getFin() : a.getDebut(), DATE_FMT);
            const QDate db = QDate::fromString(byEnd ? b.getFin() : b.getDebut(), DATE_FMT);
            return descending ? da > db : da < db;
        });
    }
    return out;
}

void Gcompetition::refreshYears() {
    QString cur = ui->fDate->currentText();
    QSet<QString> set;
    for (const Competition &c : data) set.insert(c.getDebut().right(4));
    QStringList years = set.values();
    years.sort();
    QSignalBlocker b(ui->fDate);
    ui->fDate->clear();
    ui->fDate->addItem("Toutes");
    ui->fDate->addItems(years);
    int idx = ui->fDate->findText(cur);
    if (idx >= 0) ui->fDate->setCurrentIndex(idx);
}

// ---------------------------------------------------------------- Affichage
QWidget *Gcompetition::makeBadge(const QString &statut) {
    QLabel *lab = new QLabel(statut); lab->setAlignment(Qt::AlignCenter);
    lab->setStyleSheet(QString("background:%1;color:%2;border-radius:11px;padding:3px 12px;"
                               "font-size:12px;").arg(statutBg(statut), statutFg(statut)));
    QWidget *w = new QWidget;
    QHBoxLayout *l = new QHBoxLayout(w); l->setContentsMargins(4, 0, 4, 0);
    l->addWidget(lab);
    return w;
}

QWidget *Gcompetition::makeActions(const QString &id) {
    QWidget *w = new QWidget;
    QHBoxLayout *l = new QHBoxLayout(w); l->setContentsMargins(0, 0, 0, 0); l->setSpacing(2);
    QPushButton *view = new QPushButton("👁"), *edit = new QPushButton("✏️"), *del = new QPushButton("🗑️");
    view->setToolTip("Voir les détails");
    edit->setToolTip("Modifier");
    del->setToolTip("Supprimer");
    for (QPushButton *b : {view, edit, del}) {
        b->setObjectName("act"); b->setCursor(Qt::PointingHandCursor);
        l->addWidget(b);
    }
    edit->setVisible(canEdit());    // le jury ne modifie pas
    del->setVisible(canDelete());   // seul l'administrateur supprime
    connect(view, &QPushButton::clicked, this, [this, id]() { selectRow(id); });
    connect(edit, &QPushButton::clicked, this, [this, id]() { editCompetition(id); });
    connect(del, &QPushButton::clicked, this, [this, id]() { deleteCompetition(id); });
    return w;
}

void Gcompetition::refresh(bool rebuildYears) {
    if (rebuildYears) refreshYears();
    filtered = applyFilters();
    int pages = std::max(1, (int(filtered.size()) + PER_PAGE - 1) / PER_PAGE);
    page = std::min(page, pages - 1);
    int start = page * PER_PAGE;
    int count = std::min(PER_PAGE, int(filtered.size()) - start);
    if (count < 0) count = 0;

    QTableWidget *table = ui->table;
    {
        QSignalBlocker blk(table);
        table->clearContents();
        table->setRowCount(count);
        for (int r = 0; r < count; ++r) {
            const Competition &c = filtered[start + r];
            auto mk = [](const QString &text, bool center = false) {
                QTableWidgetItem *it = new QTableWidgetItem(text);
                if (center) it->setTextAlignment(Qt::AlignCenter);
                return it;
            };
            QTableWidgetItem *idItem = mk(c.getId());
            idItem->setForeground(QColor("#1665E8"));
            QStringList problems;  // conflits de salle ou de jury : ID en rouge + info-bulle
            for (const Conflict &k : detectConflicts(c, data))
                if (k.blocking) problems << k.message;
            if (!problems.isEmpty()) {
                idItem->setForeground(QColor("#DC2626"));
                QFont bold = idItem->font();
                bold.setBold(true);
                idItem->setFont(bold);
                idItem->setToolTip("Conflit de planning :\n" + problems.join("\n"));
            }
            table->setItem(r, 0, idItem);
            table->setItem(r, 1, mk(c.getNom()));
            table->setItem(r, 2, mk(themeIcon(c.getTheme()) + "  " + c.getTheme()));
            table->setItem(r, 3, mk(c.getDebut()));
            table->setItem(r, 4, mk(c.getFin()));
            table->setItem(r, 5, mk(modeIcon(c.getMode()) + "  " + c.getMode()));
            table->setItem(r, 6, mk(QString::number(c.getMax()), true));
            table->setCellWidget(r, 7, makeBadge(c.getStatut()));
            table->setCellWidget(r, 8, makeActions(c.getId()));
            if (c.getId() == selectedId) table->selectRow(r);
        }
    }

    int total = int(filtered.size());
    ui->footer->setText(QString("Affichage de %1 à %2 sur %3 compétitions")
                        .arg(total ? start + 1 : 0).arg(start + count).arg(total));
    buildPager(pages);
    updateKpis();
    updateDetails();
    ui->aId->setText(nextId());  // l'ID proposé suit les ajouts et les suppressions
}

// Les boutons de pages sont créés ici, dans le widget vide « pagerBox » du .ui.
void Gcompetition::buildPager(int pages) {
    QHBoxLayout *pager = ui->pagerLayout;
    while (QLayoutItem *it = pager->takeAt(0)) {
        if (QWidget *w = it->widget()) { w->hide(); w->deleteLater(); }
        delete it;
    }
    QPushButton *prev = new QPushButton("‹"); prev->setObjectName("page");
    prev->setEnabled(page > 0);
    connect(prev, &QPushButton::clicked, this, [this]() { goTo(page - 1); });
    pager->addWidget(prev);
    for (int p = 0; p < pages; ++p) {
        QPushButton *b = new QPushButton(QString::number(p + 1));
        b->setObjectName(p == page ? "pageOn" : "page");
        connect(b, &QPushButton::clicked, this, [this, p]() { goTo(p); });
        pager->addWidget(b);
    }
    QPushButton *next = new QPushButton("›"); next->setObjectName("page");
    next->setEnabled(page < pages - 1);
    connect(next, &QPushButton::clicked, this, [this]() { goTo(page + 1); });
    pager->addWidget(next);
}

void Gcompetition::goTo(int p) {
    page = p;
    refresh(false);
}

void Gcompetition::updateKpis() {
    int total = int(data.size()), teams = 0, cours = 0;
    QMap<QString, int> counts;
    for (const QString &s : STATUTS) counts[s] = 0;
    for (const Competition &c : data) {
        teams += c.getMax();
        if (c.getStatut() == "En cours") ++cours;
        counts[c.getStatut()]++;
    }
    ui->kpiTotal->setText(QString::number(total));
    ui->kpiEquipes->setText(QString::number(teams));
    ui->kpiPart->setText(QString::number(teams * 4));  // estimation : 4 membres / équipe
    ui->kpiCours->setText(QString::number(cours));

    // Courbes par mois (mois de début) : équipes et participants estimés (4 par équipe).
    QDate minD, maxD;
    for (const Competition &c : data) {
        const QDate d = QDate::fromString(c.getDebut(), DATE_FMT);
        if (!d.isValid()) continue;
        if (!minD.isValid() || d < minD) minD = d;
        if (!maxD.isValid() || d > maxD) maxD = d;
    }
    QStringList months;
    QVector<double> teamsPerMonth, partPerMonth;
    if (minD.isValid()) {
        QLocale fr(QLocale::French);
        for (QDate m(minD.year(), minD.month(), 1); m <= maxD; m = m.addMonths(1)) {
            int t = 0;
            for (const Competition &c : data) {
                const QDate d = QDate::fromString(c.getDebut(), DATE_FMT);
                if (d.isValid() && d.year() == m.year() && d.month() == m.month()) t += c.getMax();
            }
            months << fr.toString(m, "MMM yy");
            teamsPerMonth << t;
            partPerMonth << t * 4;
        }
    }
    ui->statsChart->setData(months, {{"Équipes", QColor("#2563EB"), teamsPerMonth},
                                     {"Participants (estimés)", QColor("#FF6B00"), partPerMonth}});
    const int done = counts["En cours"] + counts["Terminée"];
    ui->lblTaux->setText(QString("<b>Taux de participation : %1 %</b>  (compétitions en cours ou terminées)")
                             .arg(total ? qRound(100.0 * done / total) : 0));
    ui->legCours->setText(QString::number(counts["En cours"]));
    ui->legPlanifiee->setText(QString::number(counts["Planifiée"]));
    ui->legTerminee->setText(QString::number(counts["Terminée"]));

    // Compétitions par thème : une barre par thème présent, du plus fréquent au moins fréquent.
    QMap<QString, int> byTheme;
    for (const Competition &c : data) byTheme[c.getTheme()]++;
    QList<QPair<int, QString>> sorted;
    for (auto it = byTheme.cbegin(); it != byTheme.cend(); ++it) sorted.append({it.value(), it.key()});
    std::sort(sorted.begin(), sorted.end(), [](const QPair<int, QString> &a, const QPair<int, QString> &b) {
        return a.first != b.first ? a.first > b.first : a.second < b.second;
    });
    while (QLayoutItem *it = ui->themesLayout->takeAt(0)) {
        if (QWidget *w = it->widget()) { w->hide(); w->deleteLater(); }
        delete it;
    }
    for (const auto &t : sorted) {
        QWidget *row = new QWidget;
        QHBoxLayout *l = new QHBoxLayout(row);
        l->setContentsMargins(0, 0, 0, 0);
        QLabel *name = new QLabel(themeIcon(t.second) + "  " + t.second);
        name->setMinimumWidth(150);
        QProgressBar *bar = new QProgressBar;
        bar->setObjectName("themeBar");
        bar->setRange(0, qMax(1, total));
        bar->setValue(t.first);
        bar->setTextVisible(false);
        bar->setFixedHeight(8);
        QLabel *count = new QLabel(QString::number(t.first));
        count->setMinimumWidth(20);
        count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->addWidget(name);
        l->addWidget(bar, 1);
        l->addWidget(count);
        ui->themesLayout->addWidget(row);
    }
}

void Gcompetition::updateDetails() {
    int i = indexOf(selectedId);
    if (i < 0) {
        ui->dNom->setText("Aucune sélection"); ui->dId->clear();
        ui->dBadge->clear(); ui->dBadge->setStyleSheet("");
        for (QLabel *v : {ui->dTheme, ui->dDebut, ui->dFin, ui->dMode, ui->dMax, ui->dSalle, ui->dJury,
                          ui->dDescription, ui->dSponsors})
            v->setText("-");
        return;
    }
    const Competition &c = data[i];
    QLocale fr(QLocale::French);
    ui->dIco->setText(themeIcon(c.getTheme()));
    ui->dNom->setText(c.getNom());
    int nbConflicts = 0;
    for (const Conflict &k : detectConflicts(c, data))
        if (k.blocking) ++nbConflicts;
    ui->dId->setText(nbConflicts ? QString("ID : %1   ·   <span style=\"color:#DC2626\">⛔ %2 conflit(s)</span>")
                                       .arg(c.getId()).arg(nbConflicts)
                                 : "ID : " + c.getId());
    ui->dBadge->setText(c.getStatut());
    ui->dBadge->setStyleSheet(QString("background:%1;color:%2;border-radius:10px;padding:2px 10px;"
                                      "font-size:11px;").arg(statutBg(c.getStatut()), statutFg(c.getStatut())));
    ui->dTheme->setText(c.getTheme());
    ui->dDebut->setText(fr.toString(QDate::fromString(c.getDebut(), DATE_FMT), "d MMMM yyyy"));
    ui->dFin->setText(fr.toString(QDate::fromString(c.getFin(), DATE_FMT), "d MMMM yyyy"));
    ui->dMode->setText(modeIcon(c.getMode()) + "  " + c.getMode());
    ui->dMax->setText(QString::number(c.getMax()));
    ui->dSalle->setText(c.getMode() == "En ligne" ? QString("En ligne (pas de salle)")
                                             : (c.getSalle().isEmpty() ? QString("-") : c.getSalle()));
    ui->dDescription->setText(c.getDescription().isEmpty() ? QString("-") : c.getDescription());
    ui->dSponsors->setText(c.getSponsors().isEmpty() ? QString("-") : c.getSponsors());
    ui->dJury->setText(c.getJury().isEmpty() ? QString("-") : juryMembers(c.getJury()).join(", "));
}
