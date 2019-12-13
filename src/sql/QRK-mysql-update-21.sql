SET FOREIGN_KEY_CHECKS=0;
SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;

CREATE TABLE `history` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `datetime` datetime NOT NULL,
  `data` text,
  `userId` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `rooms` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `name` text,
    `color` text,
    `sortorder` int(11) NOT NULL DEFAULT '0',
    PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `tables` (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `roomId` int(11) NOT NULL,
    `name` text,
    `color` text,
    `sortorder` int(11) NOT NULL DEFAULT '0',
    PRIMARY KEY (`id`),
    KEY `roomId` (`roomId`),
    CONSTRAINT `roomId` FOREIGN KEY (`roomId`) REFERENCES `rooms` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE  `categories` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` text NOT NULL,
  `color` text,
  `printerid` int(11),
  `image` text,
  `sortorder` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO `categories`(`id`, `name`, `color`) VALUES (1, 'Standard', '#008b8b');

ALTER TABLE `receipts` DROP COLUMN `net`;
ALTER TABLE `receipts` ADD `r2b` int(11) NOT NULL DEFAULT '0' AFTER `gross`;

ALTER TABLE `products` ADD `description` text AFTER `lastchange`;
ALTER TABLE `products` ADD `sortorder` int(11) NOT NULL DEFAULT '0' AFTER `description`;
ALTER TABLE `products` ADD `printerid` int(11) AFTER `sortorder`;
ALTER TABLE `groups` CHANGE `button` `printerid` int(11);
ALTER TABLE `groups` ADD `categoryId` int(11) NOT NULL DEFAULT '1' AFTER `visible`;
ALTER TABLE `groups` ADD `sortorder` int(11) NOT NULL DEFAULT '0' AFTER `categoryId`;
ALTER TABLE `groups` ADD CONSTRAINT `categoryId` FOREIGN KEY (`categoryId`) REFERENCES `categories` (`id`);
ALTER TABLE `reports` ADD `curfew` varchar(5) NOT NULL DEFAULT '00:00' AFTER `type`;
ALTER TABLE `reports` ADD `timestampfrom` datetime NOT NULL DEFAULT '0000-00-00T00:00:00' AFTER `curfew`;

CREATE TABLE  `tickets` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `open` tinyint(1) NOT NULL DEFAULT '1' COMMENT 'is this ticket being worked on (not payed yet)',
  `timestamp` datetime NOT NULL,
  `tableId` int(11) NOT NULL,
  `payedBy` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='they group orders per table';

CREATE TABLE  `ticketorders` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
  `ticketId` INTEGER NOT NULL REFERENCES `tickets` (`id`),
  `product` INTEGER NOT NULL REFERENCES `products` (`id`),
  `count` int(11) NOT NULL DEFAULT '1',
  `gross` double NOT NULL,
  `printed` int(11) NOT NULL DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
CREATE INDEX `orders_ticketId_index` ON `ticketorders` (`ticketId`);

CREATE TABLE `orderExtras` (
  `orderId` INTEGER NOT NULL REFERENCES `ticketorders` (`id`),
  `type` INTEGER,  /* "with" or "without" flag */
  `product` INTEGER NOT NULL REFERENCES `products` (`id`)
);
CREATE INDEX `orderExtras_orderId_index` ON `orderExtras` (`orderId`);

CREATE TABLE `orderDescs` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTO_INCREMENT,
  `type` INTEGER NOT NULL,
  `orderId` INTEGER NOT NULL,
  `description` TEXT
);
CREATE INDEX `orderDescs_orderId_index` ON `orderDescs` (`orderId`);

INSERT INTO `actionTypes` (`id`, `actionId`, `actionText`, `comment`) VALUES
(11, -1, 'Privat','PayedByText'),
(12, -2, 'Werbung','PayedByText'),
(13, -3, 'Personal','PayedByText');

CREATE TABLE `printerdefs` (
  `id` INT NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(255) NOT NULL,
  `definition` TEXT NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

CREATE TABLE `printers` (
 `id` INT NOT NULL AUTO_INCREMENT,
 `name` VARCHAR(255) NOT NULL,
 `printer` VARCHAR(255) NOT NULL,
 `definition` INT NOT NULL DEFAULT '1',
 PRIMARY KEY (`id`),
 CONSTRAINT `definition` FOREIGN KEY (`definition`) REFERENCES `printerdefs` (`id`)
 ) ENGINE=InnoDB  DEFAULT CHARSET=utf8;

UPDATE receipts set r2b=1 WHERE receiptNum IN (SELECT receiptId FROM orders WHERE product IN (SELECT id FROM products WHERE groupid=1));

SET FOREIGN_KEY_CHECKS=1;
COMMIT;
