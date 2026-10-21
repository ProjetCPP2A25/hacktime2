-- =====================================================================
--  HackTime - mise a jour de la base existante vers le NOUVEAU modele
--  (a executer UNE SEULE FOIS, connecte avec l'utilisateur hackathon, F5)
--
--  Changements du modele :
--  1. gerer devient (0,n)-(1,n) : association n,n -> nouvelle table GERER,
--     la colonne ID_EMPLOYE disparait de COMPETITION.
--  2. appartenir devient (1,n)-(0,1) avec l'attribut date_participant :
--     ID_EQUIPE devient facultatif dans PARTICIPANT, ajout de DATE_PARTICIPANT.
--  Les donnees deja saisies sont conservees.
-- =====================================================================

-- 1. Table d'association GERER (cle primaire = les deux cles etrangeres)
CREATE TABLE GERER (
    ID_EMPLOYE      NUMBER NOT NULL
                    CONSTRAINT FK_GERER_EMPLOYE REFERENCES EMPLOYE (ID_EMPLOYE) ON DELETE CASCADE,
    ID_COMPETITION  NUMBER NOT NULL
                    CONSTRAINT FK_GERER_COMPETITION REFERENCES COMPETITION (ID_COMPETITION) ON DELETE CASCADE,
    CONSTRAINT PK_GERER PRIMARY KEY (ID_EMPLOYE, ID_COMPETITION)
);

-- 2. Recopier les liens existants (qui gere quelle competition) dans GERER
INSERT INTO GERER (ID_EMPLOYE, ID_COMPETITION)
SELECT ID_EMPLOYE, ID_COMPETITION FROM COMPETITION;

-- 3. Retirer l'ancienne cle etrangere de COMPETITION
ALTER TABLE COMPETITION DROP COLUMN ID_EMPLOYE;

-- 4. PARTICIPANT : equipe facultative (0,1) + attribut date_participant
ALTER TABLE PARTICIPANT MODIFY (ID_EQUIPE NULL);
ALTER TABLE PARTICIPANT ADD (DATE_PARTICIPANT DATE);
-- si une equipe est supprimee, ses participants restent mais sans equipe
ALTER TABLE PARTICIPANT DROP CONSTRAINT FK_PARTICIPANT_EQUIPE;
ALTER TABLE PARTICIPANT ADD CONSTRAINT FK_PARTICIPANT_EQUIPE
    FOREIGN KEY (ID_EQUIPE) REFERENCES EQUIPE (ID_EQUIPE) ON DELETE SET NULL;

COMMIT;

-- Verification :
-- SELECT table_name FROM user_tables ORDER BY table_name;   -> 6 tables (avec GERER)
-- SELECT * FROM GERER;
-- DESC COMPETITION;   -> plus de colonne ID_EMPLOYE
-- DESC PARTICIPANT;   -> DATE_PARTICIPANT, ID_EQUIPE facultatif
