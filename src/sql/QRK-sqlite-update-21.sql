BEGIN TRANSACTION;

CREATE TABLE `history` (
    `id`                INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `datetime`          datetime NOT NULL,
    `data`              text,
    `userId`            INTEGER NOT NULL DEFAULT '0'
);

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
    `r2b`	INTEGER NOT NULL DEFAULT '0',
    `storno`	INTEGER NOT NULL DEFAULT '0',
    `stornoId`	INTEGER NOT NULL DEFAULT '0',
    `userId`	INTEGER NOT NULL DEFAULT '0'
);
INSERT INTO receipts SELECT `id`,`timestamp`,`infodate`,`receiptNum`,`payedBy`,`gross`,0,`storno`,`stornoId`,0 FROM `receipts_backup`;
DROP TABLE `receipts_backup`;
CREATE INDEX `receipts_stornoId_index` ON `receipts` (`stornoId`);

CREATE TABLE  `rooms` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  `name` text NOT NULL,
  `color`      	text DEFAULT '',
  `sortorder` INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE  `tables` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  `roomId` INTEGER NOT NULL,
  `name` text NOT NULL,
  `color`      	text DEFAULT '',
  `sortorder` INTEGER NOT NULL DEFAULT 0,
  CONSTRAINT `roomId` FOREIGN KEY (`roomId`) REFERENCES `rooms` (`id`)
);

CREATE TABLE  `categories` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  `name` text NOT NULL,
  `color`      	text DEFAULT '',
  `printerid` INTEGER DEFAULT NULL,
  `image`     	text DEFAULT '',
  `sortorder` INTEGER NOT NULL DEFAULT 0
);
INSERT INTO `categories`(`id`, `name`, `color`) VALUES (1, 'Standard', '#008b8b');

CREATE TEMPORARY TABLE  `groups_backup` (
    `id`        	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `name`      	text NOT NULL,
    `color`      	text DEFAULT '',
    `button`     	text DEFAULT '',
    `image`     	text DEFAULT '',
    `visible`   	tinyint(1) NOT NULL DEFAULT 1
);
INSERT INTO groups_backup SELECT `id`,`name`,`color`,`button`,`image`,`visible` FROM `groups`;
DROP TABLE `groups`;

CREATE TABLE  `groups` (
    `id`        	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    `name`      	text NOT NULL,
    `color`      	text DEFAULT '',
    `printerid` INTEGER DEFAULT NULL,
    `image`     	text DEFAULT '',
    `visible`   	tinyint(1) NOT NULL DEFAULT 1,
    `categoryId` INTEGER NOT NULL DEFAULT 1,
    `sortorder` INTEGER NOT NULL DEFAULT 0,
    CONSTRAINT `categoryId` FOREIGN KEY (`categoryId`) REFERENCES `categories` (`id`)
);
INSERT INTO groups SELECT `id`,`name`,`color`,0,`image`,`visible`, 1, 0 FROM `groups_backup`;
DROP TABLE `groups_backup`;

ALTER TABLE `products` ADD `description` text;
ALTER TABLE `products` ADD `sortorder` INTEGER NOT NULL DEFAULT 0;
ALTER TABLE `products` ADD `printerid` INTEGER DEFAULT NULL;
ALTER TABLE `reports` ADD `curfew` text NOT NULL DEFAULT "00:00";
ALTER TABLE `reports` ADD `timestampfrom` datetime NOT NULL DEFAULT '0000-00-00T00:00:00';

CREATE TABLE  `tickets` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  `open` tinyint(1) NOT NULL DEFAULT '1',
  `timestamp` datetime NOT NULL,
  `tableId` int(11) NOT NULL,
  `payedBy` int(11) NOT NULL DEFAULT '0'
);

CREATE TABLE  `ticketorders` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  `ticketId` INTEGER NOT NULL REFERENCES `tickets` (`id`),
  `product` INTEGER NOT NULL REFERENCES `products` (`id`),
  `count` int(11) NOT NULL DEFAULT '1',
  `gross` double NOT NULL,
  `printed` int(11) NOT NULL DEFAULT '0'
);

CREATE INDEX `orders_ticketId_index` ON `ticketorders` (`ticketId`);

CREATE TABLE `orderExtras` (
  `orderId` INTEGER NOT NULL REFERENCES `ticketorders` (`id`),
  `type` INTEGER,  /* "with" or "without" flag */
  `product` INTEGER NOT NULL REFERENCES `products` (`id`)
);
CREATE INDEX `orderExtras_orderId_index` ON `orderExtras` (`orderId`);

CREATE TABLE `orderDescs` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  `type` INTEGER NOT NULL,
  `orderId` INTEGER NOT NULL,
  `description` TEXT
);
CREATE INDEX `orderDescs_orderId_index` ON `orderDescs` (`orderId`);

INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,-1,'Privat','payedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,-2,'Werbung','payedByText');
INSERT INTO `actionTypes`(`id`,`actionId`,`actionText`,`comment`) VALUES (NULL,-3,'Personal','payedByText');

CREATE TABLE `printerdefs` (
        `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `name`	TEXT NOT NULL UNIQUE,
        `definition`	TEXT NOT NULL
);

CREATE TABLE `printers` (
        `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `name`	TEXT NOT NULL UNIQUE,
        `printer`	TEXT NOT NULL,
        `definition`	INTEGER NOT NULL DEFAULT 1,
        FOREIGN KEY(`definition`) REFERENCES `printerdefs`(`id`)
);

UPDATE receipts set r2b=1 WHERE receiptNum IN (SELECT receiptId FROM orders WHERE product IN (SELECT id FROM products WHERE groupid=1));

COMMIT;
