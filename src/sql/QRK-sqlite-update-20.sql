BEGIN TRANSACTION;

CREATE TABLE `receiptspay` (
    `receiptNum`	INTEGER DEFAULT NULL,
    `payedBy`	INTEGER NOT NULL DEFAULT '0',
    `gross`	double NOT NULL DEFAULT '0'
);

CREATE INDEX `receiptspay_index` ON `receiptspay` (`receiptNum`);

CREATE TEMPORARY TABLE `products_backup` (
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
INSERT INTO products_backup SELECT `id`,`itemnum`,`barcode`,`name`,`sold`,`net`,`gross`,`group`,`visible`,`completer`,`tax`,`color`,`button`,`image`, `coupon` , `stock`, `minstock` FROM `products`;
DROP TABLE `products`;

CREATE TABLE `products` (
        `id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        `itemnum`	text NOT NULL,
        `barcode`	text NOT NULL,
        `name`	text NOT NULL,
        `sold`	double NOT NULL DEFAULT 0,
        `net`	double NOT NULL,
        `gross`	double NOT NULL,
        `groupid`	INTEGER NOT NULL DEFAULT 2,
        `visible`	tinyint ( 1 ) NOT NULL DEFAULT 1,
        `completer`	tinyint ( 1 ) NOT NULL DEFAULT 1,
        `tax`	double NOT NULL DEFAULT '20',
        `color`	text DEFAULT '#808080',
        `button`	text DEFAULT '',
        `image`	text DEFAULT '',
        `coupon`	tinyint ( 1 ) NOT NULL DEFAULT 0,
        `stock`	double NOT NULL DEFAULT 0,
        `minstock`	double NOT NULL DEFAULT 0,
        `version`	INTEGER NOT NULL DEFAULT 0,
        `origin`	INTEGER NOT NULL DEFAULT 0,
        `lastchange` TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
        CONSTRAINT `groupid` FOREIGN KEY(`groupid`) REFERENCES `groups`(`id`)
);
INSERT INTO products SELECT `id`,`itemnum`,`barcode`,`name`,`sold`,`net`,`gross`,`group`,`visible`,`completer`,`tax`,`color`,`button`,`image`, `coupon`, `stock`, `minstock`, 0,0, CURRENT_TIMESTAMP FROM `products_backup`;
DROP TABLE `products_backup`;

ALTER TABLE reports ADD type INTEGER NOT NULL DEFAULT 0;

UPDATE products SET origin=id;
UPDATE taxTypes SET tax=7.7 WHERE comment="Satz-Normal" AND taxlocation="CH";
UPDATE taxTypes SET tax=3.7 WHERE comment="Satz-Besonders" AND taxlocation="CH";
UPDATE `products` SET `lastchange` = '2016-01-01 00:00:00';
COMMIT;
