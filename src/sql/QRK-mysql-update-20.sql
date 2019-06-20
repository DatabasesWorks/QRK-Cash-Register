SET FOREIGN_KEY_CHECKS=0;
SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;

CREATE TABLE `receiptspay` (
    `receiptNum`	INTEGER DEFAULT NULL,
    `payedBy`	INTEGER NOT NULL DEFAULT '0',
    `gross`	double NOT NULL DEFAULT '0',
    KEY `receiptspay_index` (`receiptNum`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

ALTER TABLE `products` ADD `version` INTEGER NOT NULL DEFAULT '0' AFTER `minstock`;
ALTER TABLE `products` ADD `origin` INTEGER NOT NULL DEFAULT '0' AFTER `version`;
ALTER TABLE `products` ADD `lastchange` TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL AFTER `origin`;
ALTER TABLE `products` CHANGE `group` `groupid` INT(11) NOT NULL DEFAULT '2';
ALTER TABLE `products` DROP INDEX `group`, ADD INDEX `groupid` (`groupid`) USING BTREE;
ALTER TABLE `products` DROP FOREIGN KEY `group`;
ALTER TABLE `products` ADD CONSTRAINT `groupid` FOREIGN KEY (`groupid`) REFERENCES `groups` (`id`);
ALTER TABLE `reports` ADD `type` INTEGER NOT NULL DEFAULT '0' AFTER `text`;

UPDATE products SET origin=id;
UPDATE taxTypes SET tax=7.7 WHERE comment="Satz-Normal" AND taxlocation="CH";
UPDATE taxTypes SET tax=3.7 WHERE comment="Satz-Besonders" AND taxlocation="CH";
UPDATE `products` SET `lastchange` = '2016-01-01 00:00:00';

SET FOREIGN_KEY_CHECKS=1;
COMMIT;
