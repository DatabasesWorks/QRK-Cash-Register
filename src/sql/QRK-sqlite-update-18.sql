BEGIN TRANSACTION;

DELETE FROM globals WHERE name = 'turnovercounter';
DELETE FROM globals WHERE name = 'lastUsedCertificate';

COMMIT;
