BEGIN TRANSACTION;

CREATE TEMPORARY TABLE  `rooms_backup` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  `name` text NOT NULL,
  `color`      	text DEFAULT '',
  `sortorder` INTEGER NOT NULL DEFAULT 0
);
INSERT INTO rooms_backup SELECT `id`,`name`,`color`,`sortorder` FROM `rooms`;
DROP TABLE `rooms`;

CREATE TABLE  `rooms` (
  `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  `name` text NOT NULL,
  `color`      	text DEFAULT '',
  `isHotel` INTEGER NOT NULL DEFAULT 0,
  `sortorder` INTEGER NOT NULL DEFAULT 0
);

INSERT INTO rooms SELECT `id`,`name`,`color`, 0, `sortorder` FROM `rooms_backup`;
DROP TABLE `rooms_backup`;

COMMIT;
