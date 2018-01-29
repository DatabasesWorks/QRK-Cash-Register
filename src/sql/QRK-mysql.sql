SET FOREIGN_KEY_CHECKS=0;
SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;

CREATE TABLE `actionTypes` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `actionId` int(11) NOT NULL,
  `actionText` text NOT NULL,
  `comment` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

INSERT INTO `actionTypes` (`id`, `actionId`, `actionText`, `comment`) VALUES
(1, 0, 'BAR', 'payedByText'),
(2, 1, 'Bankomat', 'payedByText'),
(3, 2, 'Kreditkarte', 'payedByText'),
(4, 3, 'Tagesabschluss', 'PayedByText'),
(5, 4, 'Monatsabschluss', 'PayedByText'),
(6, 5, 'Startbeleg','PayedByText'),
(7, 6, 'Kontrollbeleg','PayedByText'),
(8, 7, 'Nullbeleg','PayedByText'),
(9, 8, 'Monatsbeleg','PayedByText'),
(10, 9, 'Schlussbeleg','PayedByText');

CREATE TABLE `customer` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `receiptNum` int(11) DEFAULT NULL,
  `text` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

CREATE TABLE `journal` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `version` text NOT NULL,
  `cashregisterid` text NOT NULL,
  `datetime` datetime NOT NULL,
  `text` text,
  `userId` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `dep` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `receiptNum` int(11),
  `data` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

CREATE TABLE `globals` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` text NOT NULL,
  `value` int(11) DEFAULT NULL,
  `strValue` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `groups` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` text NOT NULL,
  `color` text,
  `button` text,
  `image` text,
  `visible` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

INSERT INTO `groups` (`id`, `name`, `color`, `button`, `image`, `visible`) VALUES
(1, 'auto', '', '', '', 0),
(2, 'Standard', '#008b8b', '', '', 1);

CREATE TABLE `orders` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `receiptId` int(11) NOT NULL,
  `product` int(11) NOT NULL,
  `count` double NOT NULL DEFAULT '1',
  `discount` double NOT NULL DEFAULT '0',
  `net` double NOT NULL,
  `gross` double NOT NULL,
  `tax` double NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `product` (`product`),
  KEY `orders_receiptId_index` (`receiptId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `products` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `itemnum` text NOT NULL,
  `barcode` text NOT NULL,
  `name` text NOT NULL,
  `sold` double NOT NULL DEFAULT '0',
  `net` double NOT NULL,
  `gross` double NOT NULL,
  `group` int(11) NOT NULL DEFAULT '2',
  `visible` tinyint(1) NOT NULL DEFAULT '1',
  `completer` tinyint(1) NOT NULL DEFAULT '1',
  `tax` double NOT NULL DEFAULT '20',
  `color` varchar(255) DEFAULT '#808080',
  `button` varchar(255) DEFAULT '',
  `image` varchar(255) DEFAULT '',
  `coupon` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `group` (`group`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

CREATE TABLE `receipts` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `timestamp` datetime NOT NULL,
  `infodate` datetime NOT NULL,
  `receiptNum` int(11) DEFAULT NULL,
  `payedBy` int(11) NOT NULL DEFAULT '0',
  `gross` double NOT NULL DEFAULT '0',
  `net` double NOT NULL DEFAULT '0',
  `storno` int(11) NOT NULL DEFAULT '0',
  `stornoId` int(11) NOT NULL DEFAULT '0',
  `userId` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `receipts_stornoId_index` (`stornoId`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

CREATE TABLE `reports` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `receiptNum` int(11) DEFAULT NULL,
  `timestamp` datetime NOT NULL,
  `text` text,
  PRIMARY KEY (`id`),
  KEY `reports_receiptNum_index` (`receiptNum`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

CREATE TABLE `taxTypes` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `tax` double DEFAULT NULL,
  `comment` text,
  `taxlocation` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

--
-- Dumping data for table `taxTypes`
--

INSERT INTO `taxTypes` (`id`, `tax`, `comment`, `taxlocation`) VALUES
(1, 20, 'Satz-Normal', 'AT'),
(2, 10, 'Satz-Ermaessigt-1', 'AT'),
(3, 13, 'Satz-Ermaessigt-2', 'AT'),
(4, 19, 'Satz-Besonders', 'AT'),
(5, 0, 'Satz-Null', 'AT'),
(6, 19, 'Satz-Normal', 'DE'),
(7, 7, 'Satz-Ermaessigt-1', 'DE'),
(8, 0, 'Satz-Null', 'DE'),
(9, 8, 'Satz-Normal', 'CH'),
(10, 2.5, 'Satz-Ermaessigt-1', 'CH'),
(11, 3.8, 'Satz-Besonders', 'CH'),
(12, 0, 'Satz-Null', 'CH');

ALTER TABLE `orders`
  ADD CONSTRAINT `product` FOREIGN KEY (`product`) REFERENCES `products` (`id`);

ALTER TABLE `products`
  ADD CONSTRAINT `group` FOREIGN KEY (`group`) REFERENCES `groups` (`id`);

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
