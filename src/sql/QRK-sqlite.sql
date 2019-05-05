CREATE TABLE `actionTypes` (
        `id`	INTEGER PRIMARY KEY AUTOINCREMENT,
        `actionId`	INTEGER NOT NULL,
        `actionText`	TEXT NOT NULL,
        `comment`	TEXT
);

INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,0,'BAR','payedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,1,'Bankomat','payedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,2,'Kreditkarte','payedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,3,'Tagesabschluss','PayedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,4,'Monatsabschluss','PayedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,5,'Startbeleg','PayedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,6,'Kontrollbeleg','PayedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,7,'Sammelbeleg','PayedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,8,'Monatsbeleg','PayedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,9,'Schlussbeleg','PayedByText');

CREATE TABLE `taxTypes` (
        `id`        INTEGER PRIMARY KEY AUTOINCREMENT,
        `tax`       double,
        `comment`   TEXT,
        `taxlocation`   TEXT
);

INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,20.0,'Satz-Normal','AT');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,10.0,'Satz-Ermaessigt-1','AT');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,13.0,'Satz-Ermaessigt-2','AT');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,19.0,'Satz-Besonders','AT');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,0.0,'Satz-Null','AT');

INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,19.0,'Satz-Normal','DE');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,7.0,'Satz-Ermaessigt-1','DE');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,0.0,'Satz-Null','DE');

INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,8.0,'Satz-Normal','CH');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,2.5,'Satz-Ermaessigt-1','CH');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,3.8,'Satz-Besonders','CH');
INSERT INTO `taxTypes`(`id`,`tax`,`comment`,`taxlocation`) VALUES (NULL,0.0,'Satz-Null','CH');

CREATE TABLE `globals` (
    `id`                INTEGER PRIMARY KEY AUTOINCREMENT,
    `name`              text NOT NULL,
    `value`             INTEGER,
    `strValue`          text
);
INSERT INTO `globals`(`name`,`value`) VALUES ('demomode',1);

CREATE TABLE  `groups` (
    `id`        	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `name`      	text NOT NULL,
    `color`      	text DEFAULT '',
    `button`     	text DEFAULT '',
    `image`     	text DEFAULT '',
    `visible`   	tinyint(1) NOT NULL DEFAULT 1
);
INSERT INTO `groups`(`name`,`visible`) VALUES ('auto', 0);
INSERT INTO `groups`(`name`,`visible`) VALUES ('Default', 0);

CREATE TABLE `products` (
    `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `itemnum`	text NOT NULL,
    `barcode`	text NOT NULL,
    `name`	text NOT NULL,
    `sold`	double NOT NULL DEFAULT 0,
    `net`	double NOT NULL,
    `gross`	double NOT NULL,
    `group`	INTEGER NOT NULL DEFAULT 2,
    `visible`	tinyint(1) NOT NULL DEFAULT 1,
    `completer`	tinyint(1) NOT NULL DEFAULT 1,
    `tax`	double NOT NULL DEFAULT '20',
    `color`     text DEFAULT '#808080',
    `button`    text DEFAULT '',
    `image`     text DEFAULT '',
    `coupon`	tinyint(1) NOT NULL DEFAULT 0,
    `stock`	double NOT NULL DEFAULT 0,
    `minstock`	double NOT NULL DEFAULT 0,
    CONSTRAINT `group` FOREIGN KEY (`group`) REFERENCES `groups` (`id`)
);

CREATE TABLE `orders` (
    `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `receiptId`	INTEGER NOT NULL,
    `product` INTEGER NOT NULL,
    `count`	double NOT NULL DEFAULT '1',
    `discount`	double NOT NULL DEFAULT '0',
    `net`	double NOT NULL,
    `gross`	double NOT NULL,
    `tax`	double NOT NULL DEFAULT '0.0',
    CONSTRAINT `product` FOREIGN KEY(`product`) REFERENCES `products` ( `id` )
);

CREATE INDEX `orders_receiptId_index` ON `orders` (`receiptId`);

CREATE TABLE `receipts` (
    `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `timestamp`	datetime NOT NULL,
    `infodate`	datetime NOT NULL,
    `receiptNum`	INTEGER DEFAULT NULL,
    `payedBy`	INTEGER NOT NULL DEFAULT '0',
    `gross`	double NOT NULL DEFAULT '0',
    `net`	double NOT NULL DEFAULT '0',
    `storno`	INTEGER NOT NULL DEFAULT '0',
    `stornoId`	INTEGER NOT NULL DEFAULT '0',
    `userId`	INTEGER NOT NULL DEFAULT '0'
);

CREATE INDEX `receipts_stornoId_index` ON `receipts` (`stornoId`);

CREATE TABLE `customer` (
    `id`                INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `receiptNum`	INTEGER DEFAULT NULL,
    `text`              text
);

CREATE TABLE `journal` (
    `id`                INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `version`           text NOT NULL,
    `cashregisterid`    text NOT NULL,
    `datetime`          datetime NOT NULL,
    `data`              text,
    `checksum`          text,
    `userId`            INTEGER NOT NULL DEFAULT '0'
);

CREATE TABLE `dep` (
        `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `receiptNum`	INTEGER,
        `data`	text
);

CREATE TABLE `reports` (
        `id`            INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `receiptNum`	INTEGER,
        `timestamp`	datetime NOT NULL,
        `text`          text
);

CREATE INDEX `reports_receiptNum_index` ON `reports` (`receiptNum`);

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
