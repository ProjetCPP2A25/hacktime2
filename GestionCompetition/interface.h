// =====================================================================
//  Classe Gcompetition : l'interface du module « Gestion des compétitions ».
//  Une seule fenêtre avec deux pages :
//    page 0 : la connexion, dessinée dans login.ui
//    page 1 (PComp) : la gestion des compétitions, dessinée dans interface.ui
//  Fonctionnel : connexion, CRUD (Ajouter, Afficher, Modifier, Supprimer)
//  et le métier recherche / filtres / tri.
//  Les autres boutons sont seulement affichés (le clic ne fait rien).
// =====================================================================
#ifndef INTERFACE_H
#define INTERFACE_H

#include <QMainWindow>
#include <QMap>
#include <QPixmap>
#include <QStringList>
#include <QVector>
#include "competition.h"

class QPropertyAnimation;

QT_BEGIN_NAMESPACE
namespace Ui { class Gcompetition; class Login; }
QT_END_NAMESPACE

class Gcompetition : public QMainWindow
{
    Q_OBJECT

public:
    explicit Gcompetition(QWidget *parent = nullptr);
    ~Gcompetition();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void seConnecter();               // bouton « Se connecter » (login.ui)
    void on_btnSave_clicked();        // Ajouter / Modifier
    void on_btnClear_clicked();       // vider le formulaire
    void on_btnDetails_clicked();     // détails de la compétition sélectionnée

private:
    // ----- login
    void preparerLogin();
    void afficherLogin();
    void deconnexion();
    bool peutModifier() const;        // Administrateur, Organisateur
    bool peutSupprimer() const;       // Administrateur

    // ----- CRUD
    void afficher();                  // lit la base et remplit le tableau
    void remplirTableau();
    void chargerFormulaire(int id);   // ✏️ : la compétition dans le formulaire de droite
    void supprimer(int id);           // 🗑️
    Competition lireFormulaire() const;

    // ----- MÉTIER : recherche, filtres et tri
    void appliquerFiltres();
    void remplirAnnees();
    void reinitialiserFiltres();

    // ----- affichage seulement
    void preparerFenetre();
    void afficherDetails();
    void mettreAJourStatistiques();
    void dessinerCourbes(QWidget *zone);
    void construirePages(int nbPages);
    QWidget *creerBadge(const QString &statut);
    QWidget *creerActions(int id);
    int indexDe(int id) const;

    // salle, jury, description, sponsors : pas dans la table COMPETITION,
    // gardés dans competitions_extra.json (clé = ID)
    void chargerInfos();
    void enregistrerInfos();

    Ui::Gcompetition *ui;                 // interface.ui
    Ui::Login *uiLogin;                   // login.ui
    QWidget *pageLogin = nullptr;         // page qui reçoit login.ui
    QString role;                         // rôle connecté
    QVector<Competition> liste;           // compétitions lues dans la base
    QVector<Competition> filtree;         // après recherche, filtres et tri (tableau)
    QMap<int, QStringList> infos;         // ID -> {salle, jury, description, sponsors}
    int idSelectionne = 0;
    int idEnModification = 0;             // 0 = ajout, sinon ID modifié
    int page = 0;

    QStringList mois;                     // données des courbes (statistiques)
    QVector<double> equipesParMois, participantsParMois;

    QPixmap fondLogin, fondLoginAjuste;   // image de fond de la page de connexion
    QPropertyAnimation *clignotementLogin = nullptr;
};

#endif // INTERFACE_H
