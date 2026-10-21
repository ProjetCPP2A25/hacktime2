-- =====================================================================
--  HackTime - base de donnees commune (Oracle 11g XE)
--  Modele relationnel valide par le tuteur :
--    Employe     (id_employe, nom, prenom, email, role, login, mdp)
--    Competition (id_competition, nom, theme, date_debut, date_fin,
--                 nb_max_membres, statut, mode)
--    Gerer       (#id_employe, #id_competition)          <- association n,n
--    Atelier     (id_atelier, titre, description, date_atelier, duree,
--                 salle, capacite, #id_competition)
--    Equipe      (id_equipe, nom_equipe, date_inscription, #id_competition)
--    Participant (id_participant, nom, prenom, email, telephone,
--                 date_participant, #id_equipe)
--
--  Execution : SQL Developer, connecte avec l'utilisateur du projet,
--  bouton "Executer le script" (F5).
--
--  Remarques :
--  - "mode" est un mot reserve d'Oracle : la colonne s'appelle MODE_COMP.
--  - Chaque cle primaire est auto-incrementee (sequence + trigger),
--    comme recommande dans l'atelier (Oracle 11g n'a pas IDENTITY).
--  - Ordre de creation : d'abord les tables sans cle etrangere.
-- =====================================================================

-- ---------------------------------------------------------------------
-- EMPLOYE
-- ---------------------------------------------------------------------
CREATE TABLE EMPLOYE (
    ID_EMPLOYE  NUMBER        CONSTRAINT PK_EMPLOYE PRIMARY KEY,
    NOM         VARCHAR2(50)  NOT NULL,
    PRENOM      VARCHAR2(50)  NOT NULL,
    EMAIL       VARCHAR2(100) CONSTRAINT UQ_EMPLOYE_EMAIL UNIQUE,
    ROLE        VARCHAR2(30)  NOT NULL
                CONSTRAINT CK_EMPLOYE_ROLE CHECK (ROLE IN ('ADMINISTRATEUR', 'ORGANISATEUR', 'JURY')),
    LOGIN       VARCHAR2(30)  NOT NULL CONSTRAINT UQ_EMPLOYE_LOGIN UNIQUE,
    MDP         VARCHAR2(64)  NOT NULL
);

-- ---------------------------------------------------------------------
-- COMPETITION
-- ---------------------------------------------------------------------
CREATE TABLE COMPETITION (
    ID_COMPETITION  NUMBER        CONSTRAINT PK_COMPETITION PRIMARY KEY,
    NOM             VARCHAR2(100) NOT NULL,
    THEME           VARCHAR2(40)  NOT NULL,
    DATE_DEBUT      DATE          NOT NULL,
    DATE_FIN        DATE          NOT NULL,
    NB_MAX_MEMBRES  NUMBER(3)     NOT NULL
                    CONSTRAINT CK_COMPETITION_MAX CHECK (NB_MAX_MEMBRES BETWEEN 1 AND 50),
    STATUT          VARCHAR2(15)  DEFAULT 'PLANIFIEE' NOT NULL
                    CONSTRAINT CK_COMPETITION_STATUT CHECK (STATUT IN ('PLANIFIEE', 'EN_COURS', 'TERMINEE')),
    MODE_COMP       VARCHAR2(15)  NOT NULL
                    CONSTRAINT CK_COMPETITION_MODE CHECK (MODE_COMP IN ('PRESENTIEL', 'EN_LIGNE', 'HYBRIDE')),
    CONSTRAINT CK_COMPETITION_DATES CHECK (DATE_FIN >= DATE_DEBUT)
);

-- ---------------------------------------------------------------------
-- GERER  (Employe 0,n --gerer-- 1,n Competition) : association n,n -> table
-- clé primaire = les deux clés étrangères
-- ---------------------------------------------------------------------
CREATE TABLE GERER (
    ID_EMPLOYE      NUMBER NOT NULL
                    CONSTRAINT FK_GERER_EMPLOYE REFERENCES EMPLOYE (ID_EMPLOYE) ON DELETE CASCADE,
    ID_COMPETITION  NUMBER NOT NULL
                    CONSTRAINT FK_GERER_COMPETITION REFERENCES COMPETITION (ID_COMPETITION) ON DELETE CASCADE,
    CONSTRAINT PK_GERER PRIMARY KEY (ID_EMPLOYE, ID_COMPETITION)
);

-- ---------------------------------------------------------------------
-- ATELIER  (Competition 1,n --organiser-- 1,1 Atelier)
-- ---------------------------------------------------------------------
CREATE TABLE ATELIER (
    ID_ATELIER      NUMBER        CONSTRAINT PK_ATELIER PRIMARY KEY,
    TITRE           VARCHAR2(100) NOT NULL,
    DESCRIPTION     VARCHAR2(500),
    DATE_ATELIER    DATE          NOT NULL,
    DUREE           NUMBER(4)     CONSTRAINT CK_ATELIER_DUREE CHECK (DUREE > 0),
    SALLE           VARCHAR2(50),
    CAPACITE        NUMBER(4)     CONSTRAINT CK_ATELIER_CAPACITE CHECK (CAPACITE > 0),
    ID_COMPETITION  NUMBER        NOT NULL
                    CONSTRAINT FK_ATELIER_COMPETITION REFERENCES COMPETITION (ID_COMPETITION)
                    ON DELETE CASCADE
);

-- ---------------------------------------------------------------------
-- EQUIPE  (Competition 2,n --s'inscrire-- 1,1 Equipe)
-- ---------------------------------------------------------------------
CREATE TABLE EQUIPE (
    ID_EQUIPE         NUMBER       CONSTRAINT PK_EQUIPE PRIMARY KEY,
    NOM_EQUIPE        VARCHAR2(50) NOT NULL,
    DATE_INSCRIPTION  DATE         DEFAULT SYSDATE NOT NULL,
    ID_COMPETITION    NUMBER       NOT NULL
                      CONSTRAINT FK_EQUIPE_COMPETITION REFERENCES COMPETITION (ID_COMPETITION)
                      ON DELETE CASCADE
);

-- ---------------------------------------------------------------------
-- PARTICIPANT  (Equipe 1,n --appartenir-- 0,1 Participant)
-- 0,1 : un participant peut ne pas (encore) avoir d'equipe -> ID_EQUIPE facultatif.
-- DATE_PARTICIPANT = attribut de l'association « appartenir ».
-- ---------------------------------------------------------------------
CREATE TABLE PARTICIPANT (
    ID_PARTICIPANT    NUMBER        CONSTRAINT PK_PARTICIPANT PRIMARY KEY,
    NOM               VARCHAR2(50)  NOT NULL,
    PRENOM            VARCHAR2(50)  NOT NULL,
    EMAIL             VARCHAR2(100) CONSTRAINT UQ_PARTICIPANT_EMAIL UNIQUE,
    TELEPHONE         VARCHAR2(20),
    DATE_PARTICIPANT  DATE,
    ID_EQUIPE         NUMBER
                      CONSTRAINT FK_PARTICIPANT_EQUIPE REFERENCES EQUIPE (ID_EQUIPE)
                      ON DELETE SET NULL
);

-- ---------------------------------------------------------------------
-- Auto-incrementation des cles primaires : une sequence + un trigger par table
-- ---------------------------------------------------------------------
CREATE SEQUENCE SEQ_EMPLOYE     START WITH 1 INCREMENT BY 1 NOCACHE;
CREATE SEQUENCE SEQ_COMPETITION START WITH 1 INCREMENT BY 1 NOCACHE;
CREATE SEQUENCE SEQ_ATELIER     START WITH 1 INCREMENT BY 1 NOCACHE;
CREATE SEQUENCE SEQ_EQUIPE      START WITH 1 INCREMENT BY 1 NOCACHE;
CREATE SEQUENCE SEQ_PARTICIPANT START WITH 1 INCREMENT BY 1 NOCACHE;

CREATE OR REPLACE TRIGGER TRG_EMPLOYE_ID
BEFORE INSERT ON EMPLOYE FOR EACH ROW
BEGIN
    IF :NEW.ID_EMPLOYE IS NULL THEN
        SELECT SEQ_EMPLOYE.NEXTVAL INTO :NEW.ID_EMPLOYE FROM DUAL;
    END IF;
END;
/

CREATE OR REPLACE TRIGGER TRG_COMPETITION_ID
BEFORE INSERT ON COMPETITION FOR EACH ROW
BEGIN
    IF :NEW.ID_COMPETITION IS NULL THEN
        SELECT SEQ_COMPETITION.NEXTVAL INTO :NEW.ID_COMPETITION FROM DUAL;
    END IF;
END;
/

CREATE OR REPLACE TRIGGER TRG_ATELIER_ID
BEFORE INSERT ON ATELIER FOR EACH ROW
BEGIN
    IF :NEW.ID_ATELIER IS NULL THEN
        SELECT SEQ_ATELIER.NEXTVAL INTO :NEW.ID_ATELIER FROM DUAL;
    END IF;
END;
/

CREATE OR REPLACE TRIGGER TRG_EQUIPE_ID
BEFORE INSERT ON EQUIPE FOR EACH ROW
BEGIN
    IF :NEW.ID_EQUIPE IS NULL THEN
        SELECT SEQ_EQUIPE.NEXTVAL INTO :NEW.ID_EQUIPE FROM DUAL;
    END IF;
END;
/

CREATE OR REPLACE TRIGGER TRG_PARTICIPANT_ID
BEFORE INSERT ON PARTICIPANT FOR EACH ROW
BEGIN
    IF :NEW.ID_PARTICIPANT IS NULL THEN
        SELECT SEQ_PARTICIPANT.NEXTVAL INTO :NEW.ID_PARTICIPANT FROM DUAL;
    END IF;
END;
/

-- ---------------------------------------------------------------------
-- Donnees de depart : les 3 comptes de l'application
-- ---------------------------------------------------------------------
INSERT INTO EMPLOYE (NOM, PRENOM, EMAIL, ROLE, LOGIN, MDP)
VALUES ('Admin', 'HackTime', 'admin@hacktime.tn', 'ADMINISTRATEUR', 'hacktime123', 'hacktime123');
INSERT INTO EMPLOYE (NOM, PRENOM, EMAIL, ROLE, LOGIN, MDP)
VALUES ('Organisateur', 'HackTime', 'orga@hacktime.tn', 'ORGANISATEUR', 'hacktime', 'hacktime');
INSERT INTO EMPLOYE (NOM, PRENOM, EMAIL, ROLE, LOGIN, MDP)
VALUES ('Jury', 'HackTime', 'jury@hacktime.tn', 'JURY', 'hacktime321', 'hacktime321');

-- quelques competitions de test, gerees par l'administrateur (table GERER)
INSERT INTO COMPETITION (NOM, THEME, DATE_DEBUT, DATE_FIN, NB_MAX_MEMBRES, STATUT, MODE_COMP)
VALUES ('Hack for a Better Future', 'Environnement', DATE '2026-11-12', DATE '2026-11-14', 5, 'PLANIFIEE', 'PRESENTIEL');
INSERT INTO COMPETITION (NOM, THEME, DATE_DEBUT, DATE_FIN, NB_MAX_MEMBRES, STATUT, MODE_COMP)
VALUES ('HealthTech Innovation', 'Sante', DATE '2026-10-15', DATE '2026-10-25', 4, 'EN_COURS', 'HYBRIDE');
INSERT INTO COMPETITION (NOM, THEME, DATE_DEBUT, DATE_FIN, NB_MAX_MEMBRES, STATUT, MODE_COMP)
VALUES ('AI for Good', 'Intelligence artificielle', DATE '2026-03-25', DATE '2026-03-27', 4, 'TERMINEE', 'EN_LIGNE');

INSERT INTO GERER (ID_EMPLOYE, ID_COMPETITION) VALUES (1, 1);
INSERT INTO GERER (ID_EMPLOYE, ID_COMPETITION) VALUES (1, 2);
INSERT INTO GERER (ID_EMPLOYE, ID_COMPETITION) VALUES (1, 3);

COMMIT;

-- Verification :
-- SELECT table_name FROM user_tables ORDER BY table_name;   -> 6 tables
-- SELECT * FROM COMPETITION;
