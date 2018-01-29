BEGIN TRANSACTION;

CREATE TEMPORARY TABLE `receipts_backup` (
    `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `timestamp`	datetime NOT NULL,
    `infodate`	datetime NOT NULL,
    `receiptNum`	int(11) DEFAULT NULL,
    `payedBy`	int(11) NOT NULL DEFAULT '0',
    `gross`	double NOT NULL DEFAULT '0',
    `net`	double NOT NULL DEFAULT '0',
    `storno`	int(11) NOT NULL DEFAULT '0',
    `stornoId`	int(11) NOT NULL DEFAULT '0'
);
INSERT INTO receipts_backup SELECT `id`,`timestamp`,`infodate`,`receiptNum`,`payedBy`,`gross`,`net`,`storno`,`stornoId` FROM `receipts`;
DROP TABLE `receipts`;

CREATE TABLE `receipts` (
    `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `timestamp`	datetime NOT NULL,
    `infodate`	datetime NOT NULL,
    `receiptNum`	int(11) DEFAULT NULL,
    `payedBy`	int(11) NOT NULL DEFAULT '0',
    `gross`	double NOT NULL DEFAULT '0',
    `net`	double NOT NULL DEFAULT '0',
    `storno`	INTEGER NOT NULL DEFAULT '0',
    `stornoId`	INTEGER NOT NULL DEFAULT '0',
    `userId`	INTEGER NOT NULL DEFAULT '0'
);
INSERT INTO receipts SELECT `id`,`timestamp`,`infodate`,`receiptNum`,`payedBy`,`gross`,`net`,`storno`,`stornoId`,0 FROM `receipts_backup`;
DROP TABLE `receipts_backup`;
CREATE INDEX `receipts_stornoId_index` ON `receipts` (`stornoId`);

CREATE TEMPORARY TABLE `journal_backup` (
    `id`                INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `version`           text NOT NULL,
    `cashregisterid`    text NOT NULL,
    `datetime`          datetime NOT NULL,
    `text`              text
);
INSERT INTO journal_backup SELECT `id`,`version`,`cashregisterid`,`datetime`,`text` FROM `journal`;
DROP TABLE `journal`;

CREATE TABLE `journal` (
    `id`                INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `version`           text NOT NULL,
    `cashregisterid`    text NOT NULL,
    `datetime`          datetime NOT NULL,
    `text`              text,
    `userId`            INTEGER NOT NULL DEFAULT '0'
);
INSERT INTO journal SELECT `id`,`version`,`cashregisterid`,`datetime`,`text`,0 FROM `journal_backup`;
DROP TABLE `journal_backup`;

CREATE TABLE `permissions` (
        `ID`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `permKey`	TEXT NOT NULL,
        `permName`	TEXT NOT NULL UNIQUE,
        CONSTRAINT `permKey` UNIQUE (`permKey`)
);

CREATE TABLE `role_perms` (
        `ID`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `roleID`	INTEGER NOT NULL,
        `permID`	INTEGER NOT NULL,
        `value`	INTEGER NOT NULL DEFAULT 0,
        `addDate`	datetime NOT NULL,
        CONSTRAINT `roleID` UNIQUE (`roleID`, `permID`)
);

CREATE TABLE `roles` (
        `ID`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `roleName`	TEXT NOT NULL UNIQUE,
        CONSTRAINT `roleName` UNIQUE (`roleName`)
);

CREATE TABLE `user_perms` (
        `ID`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `userID`	INTEGER NOT NULL,
        `permID`	INTEGER NOT NULL,
        `value`	INTEGER NOT NULL DEFAULT 0,
        `addDate`	datetime NOT NULL,
        CONSTRAINT `userId` UNIQUE (`userID`, `permID`)
);

CREATE TABLE `user_roles` (
        `ID`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `userID`	INTEGER NOT NULL,
        `roleID`	INTEGER NOT NULL,
        `addDate`	datetime NOT NULL,
        CONSTRAINT `userId` UNIQUE (`userID`, `roleID`)
);

CREATE TABLE `users` (
        `ID`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `username`	TEXT NOT NULL UNIQUE,
        `displayname`	TEXT NOT NULL,
        `password`	TEXT NOT NULL,
        `value`	INTEGER NOT NULL DEFAULT 0,
        `gender`	INTEGER NOT NULL DEFAULT 0,
        `avatar`	TEXT,
        `addDate`	datetime NOT NULL
);

INSERT INTO `permissions` (ID,permKey,permName) VALUES
    (1,'register_access','Kasse Zugang'),
    (2,'register_r2b','Kasse Bon zu Rechnung'),
    (3,'documents_access','Dokumente Zugang'),
    (4,'documents_cancellation','Dokumente stornieren'),
    (5,'tasks_access','Aufgaben Zugang'),
    (6,'tasks_create_eod','Aufgaben Tagesabschluss erstellen'),
    (7,'tasks_create_eom','Aufgaben Monatsabschluss erstellen'),
    (8,'manager_access','Manager Zugang'),
    (9,'settings_access','Einstellungen Zugang '),
    (10,'settings_view_masterdata','Einstellungen Stammdaten lesen'),
    (11,'settings_edit_masterdata','Einstellungen Stammdaten ändern'),
    (12,'settings_view_printer','Einstellungen Druckereinstellungen lesen'),
    (13,'settings_edit_printer','Einstellungen Druckereinstellungen ändern'),
    (14,'settings_view_receipt_printer','Einstellungen BON Druckereinstellungen lesen'),
    (15,'settings_edit_receipt_printer','Einstellungen BON Druckereinstellungen ändern'),
    (16,'settings_view_receipt','Einstellungen KassaBON lesen'),
    (17,'settings_edit_receipt','Einstellungen KassaBON ändern'),
    (18,'settings_view_receipt_text','Einstellungen KassaBON Texte lesen'),
    (19,'settings_edit_receipt_text','Einstellungen KassaBON Texte ändern'),
    (20,'settings_view_paths','Einstellungen Verzeichnispfade lesen'),
    (21,'settings_edit_paths','Einstellungen Verzeichnispfade ändern'),
    (22,'settings_view_extra','Einstellungen Extra Daten lesen'),
    (23,'settings_edit_extra','Einstellungen Extra Daten ändern'),
    (24,'settings_view_importserver','Einstellungen Importservereinstellungen lesen'),
    (25,'settings_edit_importserver','Einstellungen Importservereinstellungen ändern'),
    (26,'settings_view_see','Einstellungen Signatur Erstellungseinheit lesen'),
    (27,'settings_edit_see','Einstellungen Signatur Erstellungseinheit ändern'),
    (28,'admin_access','Administration Zugang'),
    (29,'admin_edit_user','Administration Benutzer ändern'),
    (30,'admin_edit_userroles','Administration Benutzerrollen ändern'),
    (31,'admin_edit_userperms','Administration Benutzerberechtigungen ändern'),
    (32,'admin_create_user','Administration Benutzer erstellen'),
    (33,'admin_delete_user','Administration Benutzer löschen'),
    (34,'admin_edit_role','Administration Rolle ändern'),
    (35,'admin_create_role','Administration Rolle erstellen'),
    (36,'admin_delete_role','Administration Rolle löschen'),
    (37,'salesinfo_view','Verkaufsinfo Dialog lesen'),
    (38,'import_csv','Import Daten');

COMMIT;
