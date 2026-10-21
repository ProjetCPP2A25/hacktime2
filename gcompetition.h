#ifndef GCOMPETITION_H
#define GCOMPETITION_H

#include <QMainWindow>
#include <QVector>
#include "competition.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Gcompetition; }
QT_END_NAMESPACE

class Gcompetition : public QMainWindow {
    Q_OBJECT

public:
    explicit Gcompetition(QWidget *parent = nullptr);
    ~Gcompetition() override;
    void setRole(const QString &role);   // appelé après la connexion

signals:
    void loggedOut();                     // l'utilisateur s'est déconnecté

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    // ----- mise en place (une fois, au démarrage)
    void applyRole();          // droits selon le rôle connecté
    void logout();
    bool canEdit() const;      // Administrateur, Organisateur
    bool canDelete() const;    // Administrateur
    void setupNavigation();
    void setupExports();
    void setupTable();
    void setupFilters();
    void setupAddForm();

    // ----- formulaire d'ajout (colonne droite, sous les détails)
    void resetAddForm();
    void showAddError(const QString &msg);
    void submitAddForm();
    Competition formComp() const;
    void updateAddConflicts();   // métier innovant : conflits détectés pendant la saisie
    void showAllConflicts();     // rapport des conflits sur tout le planning
    void openCalendar();         // calendrier des événements (vue par mois)

    // ----- actions sur les compétitions
    void editCompetition(const QString &id);
    void deleteCompetition(const QString &id);
    void selectRow(const QString &id);
    void showDetails();
    void notImplemented(const QString &module);

    // ----- exports (PDF : planning des sponsors ; Excel : liste)
    void exportPdf();
    void exportExcel();
    void showExported(const QString &title, const QString &path);

    // ----- affichage
    void onSelect();
    void onFilter();
    void resetFilters();
    void refreshYears();
    void refresh(bool rebuildYears = true);
    void buildPager(int pages);
    void goTo(int p);
    void updateKpis();
    void updateDetails();
    QWidget *makeBadge(const QString &statut);
    QWidget *makeActions(const QString &id);

    // ----- utilitaires
    QString nextId() const;
    int indexOf(const QString &id) const;
    QVector<Competition> applyFilters() const;

    Ui::Gcompetition *ui;       // tous les widgets dessinés dans gcompetition.ui
    QVector<Competition> data, filtered;
    int page = 0;
    QString selectedId;
    QString role;               // Administrateur, Organisateur ou Jury
};

#endif // GCOMPETITION_H
