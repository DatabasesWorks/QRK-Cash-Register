SET FOREIGN_KEY_CHECKS=0;
SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;

ALTER TABLE `receipts` ADD `userId` int(11) NOT NULL DEFAULT '0' AFTER `stornoId`;
ALTER TABLE `journal` ADD `userId` int(11) NOT NULL DEFAULT '0' AFTER `text`;

CREATE TABLE `permissions` (
    `ID` bigint(20) unsigned NOT NULL auto_increment,
    `permKey` varchar(255) NOT NULL,
    `permName` varchar(255) NOT NULL,
    PRIMARY KEY  (`ID`),
    UNIQUE KEY `permKey` (`permKey`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `roles` (
    `ID` bigint(20) unsigned NOT NULL auto_increment,
    `roleName` varchar(255) NOT NULL,
    PRIMARY KEY  (`ID`),
    UNIQUE KEY `roleName` (`roleName`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `role_perms` (
    `ID` bigint(20) unsigned NOT NULL auto_increment,
    `roleID` bigint(20) NOT NULL,
    `permID` bigint(20) NOT NULL,
    `value` tinyint(1) NOT NULL default '0',
    `addDate` datetime NOT NULL,
    PRIMARY KEY  (`ID`),
    UNIQUE KEY `roleID_2` (`roleID`,`permID`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `users` (
    `ID` int(10) unsigned NOT NULL auto_increment,
    `username` varchar(255) NOT NULL,
    `displayname` varchar(255) NOT NULL,
    `password` varchar(255) NOT NULL,
    `value` tinyint(1) NOT NULL default '0',
    `gender` tinyint(1) NOT NULL default '0',
    `avatar` varchar(255),
    `addDate` datetime NOT NULL,
    PRIMARY KEY  (`ID`),
    KEY `username` (`username`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `user_perms` (
    `ID` bigint(20) unsigned NOT NULL auto_increment,
    `userID` bigint(20) NOT NULL,
    `permID` bigint(20) NOT NULL,
    `value` tinyint(1) NOT NULL default '0',
    `addDate` datetime NOT NULL,
    PRIMARY KEY  (`ID`),
    UNIQUE KEY `userID` (`userID`,`permID`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `user_roles` (
    `userID` bigint(20) NOT NULL,
    `roleID` bigint(20) NOT NULL,
    `addDate` datetime NOT NULL,
    UNIQUE KEY `userID` (`userID`,`roleID`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8;

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

SET FOREIGN_KEY_CHECKS=1;
COMMIT;
