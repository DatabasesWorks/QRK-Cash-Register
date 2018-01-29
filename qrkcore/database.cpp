/*
 * This file is part of QRK - Qt Registrier Kasse
 *
 * Copyright (C) 2015-2018 Christian Kvasny <chris@ckvsoft.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Button Design, and Idea for the Layout are lean out from LillePOS, Copyright 2010, Martin Koller, kollix@aon.at
 *
 */

#include "database.h"
#include "databasedefinition.h"
#include "utils/demomode.h"
#include "preferences/qrksettings.h"
#include "databasemanager.h"

#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDate>
#include <QStandardPaths>

QMap<QString, QString> globalStringValues;

Database::Database(QObject *parent)
    : QObject(parent)
{
}

//--------------------------------------------------------------------------------

Database::~Database()
{
}

QDateTime Database::getLastJournalEntryDate()
{
    QDateTime lastDateTime;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT DateTime FROM journal WHERE id = (SELECT MAX(id) FROM journal)");
    query.exec();
    query.next();
    lastDateTime = query.value("datetime").toDateTime();
    return lastDateTime;
}

QString Database::getLastExecutedQuery(const QSqlQuery& query)
{
    QString str = query.lastQuery();

    QMapIterator<QString, QVariant> it(query.boundValues());

    it.toBack();

    while (it.hasPrevious()) {
        it.previous();
        str.replace(it.key(), it.value().toString());
    }
    return str;
}

QString Database::getDayCounter()
{
    QDateTime dateFrom;
    QDateTime dateTo;

    dateFrom.setDate(QDate::currentDate());
    dateTo.setDate(QDate::currentDate());
    dateTo.setTime(QTime::currentTime());

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    /* Summe */
    query.prepare(QString("SELECT sum(gross) as total FROM receipts WHERE timestamp BETWEEN :fromdate AND :todate AND payedBy < 3"));
    query.bindValue(":fromdate", dateFrom.toString(Qt::ISODate));
    query.bindValue(":todate", dateTo.toString(Qt::ISODate));

    query.exec();
    query.next();

    return QString::number(query.value("total").toDouble(), 'f', 2);
}

QString Database::getMonthCounter()
{
    QDateTime dateFrom;
    QDateTime dateTo;

    dateFrom.setDate(QDate::fromString(QString("%1-%2-1")
                                       .arg(QDate::currentDate().year())
                                       .arg(QDate::currentDate().month())
                                       , "yyyy-M-d"));
    dateTo.setDate(QDate::currentDate());
    dateTo.setTime(QTime::currentTime());

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    /* Summe */
    query.prepare(QString("SELECT sum(gross) as total FROM receipts WHERE timestamp BETWEEN :fromdate AND :todate AND payedBy < 3"));
    query.bindValue(":fromdate", dateFrom.toString(Qt::ISODate));
    query.bindValue(":todate", dateTo.toString(Qt::ISODate));

    query.exec();
    query.next();

    return QString::number(query.value("total").toDouble(), 'f', 2);
}

QString Database::getYearCounter()
{
    QDateTime dateFrom;
    QDateTime dateTo;

    dateFrom.setDate(QDate::fromString(
                         QString("%1-1-1")
                         .arg(QDate::currentDate().year())
                         , "yyyy-M-d")
                     );

    dateTo.setDate(QDate::currentDate());
    dateTo.setTime(QTime::currentTime());

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    /* Summe */
    query.prepare(QString("SELECT sum(gross) as total FROM receipts WHERE timestamp BETWEEN :fromdate AND :todate AND payedBy < 3"));
    query.bindValue(":fromdate", dateFrom.toString(Qt::ISODate));
    query.bindValue(":todate", dateTo.toString(Qt::ISODate));

    query.exec();
    query.next();

    return QString::number(query.value("total").toDouble(), 'f', 2);
}

QString Database::getSalesPerPaymentSQLQueryString()
{
    return "SELECT actionTypes.actionText, orders.tax, (orders.count * orders.gross) - ((orders.count * orders.gross / 100) * orders.discount) as total from orders "
            " LEFT JOIN receipts on orders.receiptId=receipts.receiptNum"
            " LEFT JOIN actionTypes on receipts.payedBy=actionTypes.actionId"
            " WHERE receipts.timestamp between :fromDate AND :toDate AND receipts.payedBy < 3"
            " ORDER BY receipts.payedBy, orders.tax";
    /*
    return "SELECT actionTypes.actionText, orders.tax, SUM((orders.count * orders.gross) - ((orders.count * orders.gross / 100) * orders.discount)) as total from orders "
           " LEFT JOIN receipts on orders.receiptId=receipts.receiptNum"
           " LEFT JOIN actionTypes on receipts.payedBy=actionTypes.actionId"
           " WHERE receipts.timestamp between :fromDate AND :toDate AND receipts.payedBy < 3"
           " GROUP BY orders.tax, receipts.payedBy ORDER BY receipts.payedBy, orders.tax";
*/
    /*
    return "SELECT actionTypes.actionText, orders.tax, SUM((orders.count * orders.gross) - round(((orders.count * orders.gross / 100) * orders.discount),2)) as total from orders "
           " LEFT JOIN receipts on orders.receiptId=receipts.receiptNum"
           " LEFT JOIN actionTypes on receipts.payedBy=actionTypes.actionId"
           " WHERE receipts.timestamp between :fromDate AND :toDate AND receipts.payedBy < 3"
           " GROUP BY orders.tax, receipts.payedBy ORDER BY receipts.payedBy, orders.tax";
           */
}

//--------------------------------------------------------------------------------

void Database::updateProductSold(double count, QString product)
{
    if (product.startsWith("Zahlungsbeleg für Rechnung"))
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("UPDATE products SET sold=sold+:sold WHERE name=:name"));
    query.bindValue(":sold", count);
    query.bindValue(":name", QVariant(product));

    query.exec();
}

//--------------------------------------------------------------------------------

QString Database::getTaxLocation()
{
    if (globalStringValues.contains("taxlocation"))
        return globalStringValues.value("taxlocation");

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='taxlocation'");
    query.exec();
    if (query.next()) {
        QString ret = query.value(0).toString();
        if (!ret.isEmpty()) {
            globalStringValues.insert("taxlocation", ret);
            return globalStringValues.value("taxlocation");
        }
    }

    return Database::updateGlobals("taxlocation", NULL, "AT");
}

//--------------------------------------------------------------------------------

QString Database::getDefaultTax()
{
    if (globalStringValues.contains("defaulttax"))
        return globalStringValues.value("defaulttax");

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT strValue FROM globals WHERE name='defaulttax' LIMIT 1"));
    query.exec();
    if (query.next()) {
        QString ret = query.value(0).toString();
        if (!ret.isEmpty()) {
            globalStringValues.insert("defaulttax", ret);
            return globalStringValues.value("defaulttax");
        }
    }

    return Database::updateGlobals("defaulttax", NULL, "20");
}

//--------------------------------------------------------------------------------

QString Database::getShortCurrency()
{
    if (globalStringValues.contains("currency"))
        return globalStringValues.value("currency");

    QString currency = getCurrency();

    if (currency == "CHF")
        return "Fr";
    else
        return "€";
}

//--------------------------------------------------------------------------------

QString Database::getCurrency()
{
    if (!globalStringValues.contains("currency")) {
        QSqlDatabase dbc = Database::database();
        QSqlQuery query(dbc);

        query.prepare("SELECT strValue FROM globals WHERE name='currency'");
        query.exec();
        if (query.next()) {
            globalStringValues.insert("currency", query.value(0).toString());
        } else {
            return Database::updateGlobals("currency", NULL, query.value(0).toString());
        }
    }

    return globalStringValues.value("currency");
}

//--------------------------------------------------------------------------------

QString Database::getCashRegisterId()
{
    if (globalStringValues.contains("shopCashRegisterId")) {
        if (DemoMode::isDemoMode())
            return "DEMO-" + globalStringValues.value("shopCashRegisterId");
        else
            return globalStringValues.value("shopCashRegisterId");
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='shopCashRegisterId'");
    query.exec();
    if (query.next()) {
        QString id = query.value(0).toString();
        if (id.isEmpty())
            return QString();

        globalStringValues.insert("shopCashRegisterId", id);
        if (DemoMode::isDemoMode())
            return "DEMO-" + id;

        return query.value(0).toString();
    }

    return "";
}

//--------------------------------------------------------------------------------

QStringList Database::getMaximumItemSold()
{
    QStringList list;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT max(sold), name, tax, gross FROM products LIMIT 1;");
    query.exec();
    if (query.next()) {
        list	<< query.value("name").toString()
                << query.value("tax").toString()
                << query.value("gross").toString();
        return list;
    }

    list << "" << "20" << "0,00";
    return list;
}

//--------------------------------------------------------------------------------

bool Database::addCustomerText(int id, QString text)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString q = QString("INSERT INTO customer (receiptNum, text) VALUES (:receiptNum, :text)");
    bool ok = query.prepare(q);

    query.bindValue(":receiptNum", id);
    query.bindValue(":text", text);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
    }

    if (query.exec())
        return true;
    else
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();

    return false;
}

//--------------------------------------------------------------------------------

QString Database::getCustomerText(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    bool ok = query.prepare("SELECT text FROM customer WHERE receiptNum=:receiptNum");

    query.bindValue(":receiptNum", id);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
    }

    if (query.exec()) {
        if (query.next())
            return query.value("text").toString();

        return "";
    } else {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
    }

    return "";
}

//--------------------------------------------------------------------------------

int Database::getProductIdByName(QString name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString q = QString("SELECT id FROM products WHERE name=:name");
    bool ok = query.prepare(q);

    query.bindValue(":name", name);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
    }

    if (query.exec()) {
        if (query.next())
            return query.value("id").toInt();
    }

    return -1;
}

int Database::getProductIdByBarcode(QString code)
{
    if (code.isEmpty())
        return -1;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString q = QString("SELECT id FROM products WHERE barcode=:barcode");
    bool ok = query.prepare(q);
    query.bindValue(":barcode", code);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
    }

    if (query.exec()) {
        if (query.next()) {
            int id = query.value("id").toInt();
            return id;
        }
    }

    return -1;
}

bool Database::addProduct(const QList<QVariant> &data)
{
    bool productExists = Database::exists("products", data.at(0).toString());

    int visible = data.at(4).toInt();
    int group = 2;

    if (data.length() == 6)
        group = data.at(5).toInt();
    if (data.at(0).toString().startsWith("Zahlungsbeleg für Rechnung") | data.at(0).toString().startsWith("Startbeleg")) {
        if (productExists) {
            if (data.at(0).toString().startsWith("Startbeleg")) {
                qWarning() << "Function Name: " << Q_FUNC_INFO << "Ein Startbeleg ist schon vorhanden.";
                return true;
            }
            return false;
        }

        /* We do not need to display this in the Manager*/
        visible = 0;
        group = 1;
    }

    if (productExists)
        return true;


    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString q = QString("INSERT INTO products (name, itemnum, barcode, tax, net, gross, visible, `group`) VALUES (:name, '', '', :tax, :net, :gross, :visible, :group)");
    bool ok = query.prepare(q);

    query.bindValue(":name", data.at(0).toString());
    query.bindValue(":tax", data.at(1).toDouble());
    query.bindValue(":net", data.at(2).toDouble());
    query.bindValue(":gross", data.at(3).toDouble());
    query.bindValue(":visible", visible);
    query.bindValue(":group", group);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
        return false;
    }

    if (query.exec()) {
        return true;
    } else {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
    }

    return false;
}

//--------------------------------------------------------------------------------

bool Database::exists(const QString type, const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT id FROM %1 WHERE name=:name").arg(type));
    query.bindValue(":name", name);

    query.exec();
    if (query.next())
        return true;

    return false;
}

//--------------------------------------------------------------------------------

int Database::getLastReceiptNum(bool realReceipt)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    if (realReceipt)
        query.prepare("SELECT receiptNum FROM receipts WHERE id=(SELECT max(id) FROM receipts WHERE payedBy < 3);");
    else
        query.prepare("SELECT value FROM globals WHERE name='lastReceiptNum'");

    query.exec();

    if (query.next())
        return query.value(0).toInt();

    return 0;
}

QStringList Database::getLastReceipt()
{
    QStringList list;

    int lastReceipt = getLastReceiptNum();

    if (lastReceipt == 0)
        return list;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT timestamp, receiptNum, payedBy, gross FROM receipts WHERE receiptNum=%1").arg(lastReceipt));
    query.exec();
    query.next();
    list << query.value(0).toString() << query.value(1).toString() << query.value(2).toString() << query.value(3).toString();

    return list;
}

//--------------------------------------------------------------------------------

QDate Database::getLastReceiptDate()
{
    return getLastReceiptDateTime().date();
}

//--------------------------------------------------------------------------------

QDateTime Database::getLastReceiptDateTime()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT infodate FROM receipts where receiptNum IN (SELECT value FROM globals WHERE name='lastReceiptNum')");
    query.exec();
    QDateTime dt = QDateTime::currentDateTime();
    if (query.next()) {
        dt = query.value(0).toDateTime();
        return dt;
    }

    return QDateTime();
}

//--------------------------------------------------------------------------------

QString Database::getDatabaseType()
{
    // read global defintions (DB, ...)
    QrkSettings settings;

    return settings.value("DB_type").toString();
}

//--------------------------------------------------------------------------------

QString Database::getShopName()
{
    QString name;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='shopName'");
    query.exec();
    query.next();

    name = query.value(0).toString();

    return name;
}

QString Database::getShopMasterData()
{
    QString name;
    QString tmp;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='shopOwner'");
    query.exec();
    query.next();
    tmp = query.value(0).toString();

    name = (tmp.isEmpty()) ? "" : "\n" + tmp;

    query.prepare("SELECT strValue FROM globals WHERE name='shopAddress'");
    query.exec();
    query.next();
    tmp = query.value(0).toString();

    name += (tmp.isEmpty()) ? "" : "\n" + tmp;

    query.prepare("SELECT strValue FROM globals WHERE name='shopUid'");
    query.exec();
    query.next();
    tmp = query.value(0).toString();

    name += (tmp.isEmpty()) ? "" : "\n" + tmp;

    return name;
}

QString Database::getLastVersionInfo()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT version FROM journal WHERE id = (SELECT MAX(id) FROM journal)");
    query.exec();
    query.next();
    return query.value("version").toString();
}

//--------------------------------------------------------------------------------
void Database::reopen()
{
    QSqlDatabase dbc = QSqlDatabase::database("CN");

    dbc.close();
    open(false);
}

bool Database::open(bool dbSelect)
{
    const int CURRENT_SCHEMA_VERSION = 17;
    // read global defintions (DB, ...)
    QrkSettings settings;

    QString dbType = getDatabaseType();

    if (dbType.isEmpty() || dbSelect) {
        DatabaseDefinition dialog(0);

        if (dialog.exec() == QDialog::Rejected)
            return false;

        dbType = dialog.getDbType();

        settings.save2Settings("DB_type", dbType, false);
        settings.save2Settings("DB_userName", dialog.getUserName(), false);
        settings.save2Settings("DB_password", dialog.getPassword(), false);
        settings.save2Settings("DB_hostName", dialog.getHostName(), false);
    }

    QDate date = QDate::currentDate();

    QString standardDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data";

    QDir t(qApp->applicationDirPath() + "/data");
    QStringList filter;
    filter << "*.db";
    if (!t.entryList(filter, QDir::NoDotAndDotDot).empty())
        settings.save2Settings("sqliteDataDirectory", qApp->applicationDirPath() + "/data", false);

    QString dataDir = settings.value("sqliteDataDirectory", standardDataDir).toString();
    QDir dir(dataDir);

    if (!dir.exists())
        dir.mkpath(".");

    QFileInfo fi(settings.fileName());
    QString basename = fi.baseName();

    if (!QFile::exists(QString(dataDir + "/%1-%2.db").arg(date.year()).arg(basename)))
        if (QFile::exists(QString(dataDir + "/%1-%2.db").arg(date.year() - 1).arg(basename)))
            QFile::copy(QString(dataDir + "/%1-%2.db").arg(date.year() - 1).arg(basename), QString(dataDir + "/%1-%2.db").arg(date.year()).arg(basename));

    QSqlDatabase currentConnection;

    // setup database connection
    if (dbType == "QSQLITE") {
        currentConnection = QSqlDatabase::addDatabase("QSQLITE", "CN");
        currentConnection.setDatabaseName(QString(dataDir + "/%1-%2.db").arg(date.year()).arg(basename));
    } else if (dbType == "QMYSQL") {
        QString userName = settings.value("DB_userName", "QRK").toString();
        QString password = settings.value("DB_password", "").toString();
        QString hostName = settings.value("DB_hostName", "localhost").toString();

        currentConnection = QSqlDatabase::addDatabase("QMYSQL", "CN");
        currentConnection.setHostName(hostName);
        currentConnection.setUserName(userName);
        currentConnection.setPassword(password);
        currentConnection.setConnectOptions("MYSQL_OPT_RECONNECT=1;MYSQL_OPT_CONNECT_TIMEOUT=86400;MYSQL_OPT_READ_TIMEOUT=60");
    }

    bool ok = currentConnection.open();

    if (!ok) {
        QMessageBox errorDialog;
        errorDialog.setIcon(QMessageBox::Critical);
        errorDialog.addButton(QMessageBox::Ok);
        errorDialog.setText(currentConnection.lastError().text());
        errorDialog.setWindowTitle(QObject::tr("Datenbank Verbindungsfehler"));
        errorDialog.exec();
        return false;
    }

    if (dbType == "QMYSQL") {
        QSqlQuery query(currentConnection);
        query.prepare(QString("SHOW DATABASES LIKE '%1'").arg(basename));
        query.exec();
        QString errorText = "";
        if (!query.next()) {    // db does not exist
            query.prepare(QString("CREATE DATABASE %1").arg(basename));
            query.exec();
            errorText = query.lastError().text();
            qDebug() << "Function Name: " << Q_FUNC_INFO << query.lastError().text();
        }

        currentConnection.close();
        currentConnection.setDatabaseName(basename);
        if (!currentConnection.open()) {
            QMessageBox errorDialog;
            errorDialog.setIcon(QMessageBox::Critical);
            errorDialog.addButton(QMessageBox::Ok);
            errorDialog.setText(errorText + "\n" + currentConnection.lastError().text());
            errorDialog.setWindowTitle(QObject::tr("Datenbank Verbindungsfehler"));
            errorDialog.exec();
            return false;
        }
    }

    QString lastError;
    if (dbType == "QSQLITE") {
        if (QFile::exists(QString(dataDir + "/%1-%2.db-journal").arg(date.year()).arg(basename))) {
            QSqlQuery query(currentConnection);
            currentConnection.transaction();
            query.exec("create table force_journal_cleanup (id integer primary key);");
            currentConnection.rollback();
            lastError = query.lastError().text();
        }
        if (QFile::exists(QString(dataDir + "/%1-%2.db-journal").arg(date.year()).arg(basename))) {
            QMessageBox errorDialog;
            errorDialog.setIcon(QMessageBox::Critical);
            errorDialog.addButton(QMessageBox::Ok);
            errorDialog.setText(lastError + ".\n" + tr("Eventuell müssen andere geöffnete Anwendungen geschlossen werden."));
            errorDialog.setWindowTitle(QObject::tr("Datenbank Fehler"));
            errorDialog.exec();
            return false;
        }
    }

    QSqlQuery query(currentConnection);
    query.exec("SELECT 1 FROM globals");
    if (!query.next()) {    // empty DB, create all tables
        QFile f;

        if (dbType == "QSQLITE") {
            f.setFileName(":src/sql/QRK-sqlite.sql");
        } else if (dbType == "QMYSQL") {
            f.setFileName(":src/sql/QRK-mysql.sql");
            currentConnection.setConnectOptions("MYSQL_OPT_RECONNECT=1");
        }

        f.open(QIODevice::ReadOnly);
        QString cmd = f.readAll();
        QStringList commands = cmd.split(';', QString::SkipEmptyParts);
        foreach(const QString &command, commands) {
            if (command.trimmed().isEmpty())
                continue;
            bool ok = query.exec(command);
            if (!ok) {
                qWarning() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                qWarning() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
            }
        }

        query.exec("INSERT INTO globals (name, value) VALUES('lastReceiptNum', 0)");
        query.exec("INSERT INTO globals (name, strValue) VALUES('shopName', '')");
        query.exec("INSERT INTO globals (name, strValue) VALUES('backupTarget', '')");

        query.exec(QString("INSERT INTO globals (name, value) VALUES('schemaVersion', %1)")
                   .arg(CURRENT_SCHEMA_VERSION));

        query.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,text) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, 'Id\tProgrammversion\tKassen-Id\tKonfigurationsänderung\tBeschreibung\tErstellungsdatum')"));
        query.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,text) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, 'Id\tProgrammversion\tKassen-Id\tProduktposition\tBeschreibung\tMenge\tEinzelpreis\tGesamtpreis\tUSt. Satz\tErstellungsdatum')"));
        query.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,text) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, 'Id\tProgrammversion\tKassen-Id\tBeleg\tBelegtyp\tBemerkung\tNachbonierung\tBelegnummer\tDatum\tUmsatz Normal\tUmsatz Ermaessigt1\tUmsatz Ermaessigt2\tUmsatz Null\tUmsatz Besonders\tJahresumsatz bisher\tErstellungsdatum')"));
        query.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,text) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, 'Id\tProgrammversion\tKassen-Id\tBeleg-Textposition\tText\tErstellungsdatum')"));
    } else { // db already exists; check if we need to run an update
        int schemaVersion = 1;
        query.exec("SELECT value FROM globals WHERE name='schemaVersion'");
        if (query.next())
            schemaVersion = query.value("value").toInt();
        else  // schemaVersion not set in globals, must be version 1
            query.exec("INSERT INTO globals (name, value) VALUES('schemaVersion', 1)");

        if (schemaVersion > CURRENT_SCHEMA_VERSION) {
            currentConnection.close();
            QMessageBox errorDialog;
            errorDialog.setIcon(QMessageBox::Critical);
            errorDialog.addButton(QMessageBox::Ok);
            errorDialog.setText(tr("Falsches QRK Registrier Kassen Datenbank Schema.\nVerwenden Sie bitte eine neuere Version von QRK.\nBei Fragen wenden Sie sich bitte an das QRK Forum (www.ckvsoft.at)"));
            errorDialog.setWindowTitle(QObject::tr("Falsche Version"));
            errorDialog.exec();
            return false;
        }

        // run all db update scripts from the db version + 1 to what the program currently needs
        for (int i = schemaVersion + 1; i <= CURRENT_SCHEMA_VERSION; i++) {
            QFile f;

            if (dbType == "QSQLITE")
                f.setFileName(QString(":src/sql/QRK-sqlite-update-%1.sql").arg(i));
            else if (dbType == "QMYSQL")
                f.setFileName(QString(":src/sql/QRK-mysql-update-%1.sql").arg(i));

            if (!f.open(QIODevice::ReadOnly)) {
                qWarning("could not load internal update file %s", qPrintable(f.fileName()));
                return false;  // should never happen
            }

            QString cmd = f.readAll();
            QStringList commands = cmd.split(';', QString::SkipEmptyParts);
            bool ok;
            foreach(const QString &command, commands) {
                if (command.trimmed().isEmpty())
                    continue;
                ok = query.exec(command);
                if (!ok) {
                    qWarning() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                    qWarning() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
                }
            }

            // changes which are not possible in sql file needed for update-7
            if (i == 7) {
                ok = query.exec(QString("UPDATE products SET `group`=2 WHERE name NOT LIKE 'Zahlungsbeleg für Rechnung%'"));
                if (!ok) {
                    qWarning() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                    qWarning() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
                }
                ok = query.exec(QString("UPDATE products SET `group`=1 WHERE name LIKE 'Zahlungsbeleg für Rechnung%'"));
                if (!ok) {
                    qWarning() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                    qWarning() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
                }
            }
            if (i == 14) {
                ok = query.exec(QString("UPDATE journal SET `text`='%1' WHERE id=1").arg("Id\tProgrammversion\tKassen-Id\tKonfigurationsänderung\tBeschreibung\tErstellungsdatum"));
                if (!ok) {
                    qWarning() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                    qWarning() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
                }
            }
        }

        if (schemaVersion != CURRENT_SCHEMA_VERSION)
            query.exec(QString("UPDATE globals SET value=%1 WHERE name='schemaVersion'")
                       .arg(CURRENT_SCHEMA_VERSION));

    }

    if (dbType == "QSQLITE") {
        // enforce foreign key constraint
        query.exec("PRAGMA foreign_keys = 1;");
    }

    return true;
}

//--------------------------------------------------------------------------------

int Database::getPayedBy(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT payedBy FROM receipts WHERE receiptNum=%1").arg(id));
    query.exec();
    query.next();

    return query.value(0).toInt();
}

//--------------------------------------------------------------------------------

int Database::getActionTypeByName(const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT actionId FROM actionTypes WHERE actionText=:actionText"));
    query.bindValue(":actionText", name);
    query.exec();
    query.next();

    return query.value(0).toInt();
}

//--------------------------------------------------------------------------------

QString Database::getActionType(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT actionText FROM actionTypes WHERE actionId=%1").arg(id));
    query.exec();
    query.next();

    return query.value(0).toString();
}

//--------------------------------------------------------------------------------

QString Database::getTaxType(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT comment FROM taxTypes WHERE tax=%1").arg(id));
    query.exec();
    query.next();

    return query.value(0).toString();
}

//--------------------------------------------------------------------------------

void Database::setStorno(int id, int value)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("UPDATE receipts SET storno=:value WHERE receiptNum=:receiptNum");
    query.bindValue(":value", value);
    query.bindValue(":receiptNum", id);
    bool ok = query.exec();
    if (!ok) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
}

//--------------------------------------------------------------------------------

void Database::setStornoId(int sId, int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    // Beleg wurde von 'sId' storniert
    query.prepare("UPDATE receipts SET stornoId=:stornoId WHERE receiptNum=:receiptNum");
    query.bindValue(":stornoId", sId);
    query.bindValue(":receiptNum", id);
    bool ok = query.exec();
    if (!ok) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    // Beleg ist Stornobeleg von Beleg Nr: 'id'
    query.prepare("UPDATE receipts SET stornoId=:stornoId WHERE receiptNum=:receiptNum");
    query.bindValue(":stornoId", id);
    query.bindValue(":receiptNum", sId);
    ok = query.exec();
    if (!ok) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    setStorno(id);      // Beleg wurde storniert
    setStorno(sId, 2);  // Beleg ist StornoBeleg
}

//--------------------------------------------------------------------------------

int Database::getStorno(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT storno FROM receipts WHERE receiptNum=:receiptNum");
    query.bindValue(":receiptNum", id);

    bool ok = query.exec();
    if (!ok) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();
    return query.value(0).toInt();
}

//--------------------------------------------------------------------------------

int Database::getStornoId(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT stornoId FROM receipts WHERE receiptNum=:receiptNum");
    query.bindValue(":receiptNum", id);
    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();
    return query.value(0).toInt();
}

void Database::setCashRegisterInAktive()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery q(dbc);

    q.prepare("SELECT value FROM globals WHERE name=:name");
    q.bindValue(":name", "CASHREGISTER INAKTIV");
    bool ok = q.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << q.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(q);
    }
    if (q.next()) {
        if (q.value("value").toInt() == 1)
            return;
        else
            q.prepare("UPDATE globals set name=:name, value=:value;");
    } else {
        q.prepare("INSERT INTO globals (name, value) VALUES(:name, :value)");
    }

    q.bindValue(":name", "CASHREGISTER INAKTIV");
    q.bindValue(":value", 1);

    q.exec();
}

bool Database::isCashRegisterInAktive()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT value FROM globals WHERE name = 'CASHREGISTER INAKTIV'");
    query.exec();

    if (query.next())
        if (query.value(0).toInt() == 1)
            return true;

    return false;
}

void Database::resetAllData()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery q(dbc);

    q.prepare("DELETE FROM journal;");
    q.exec();

    q.prepare("DELETE FROM orders;");
    q.exec();

    q.prepare("DELETE FROM receipts;");
    q.exec();

    q.prepare("DELETE FROM reports;");
    q.exec();

    q.prepare("DELETE FROM dep;");
    q.exec();

    q.prepare("DELETE FROM products WHERE `group`=1;");
    q.exec();

    q.prepare("DELETE FROM globals WHERE `name`='PrivateTurnoverKey';");
    q.exec();

    q.prepare("UPDATE globals SET value = 0 WHERE name = 'lastReceiptNum';");
    q.exec();

    q.prepare("DELETE FROM globals WHERE `name`='certificate';");
    q.exec();

    q.prepare("DELETE FROM globals WHERE `name`='DEP';");
    q.exec();

    q.prepare("DELETE FROM globals WHERE `name`='lastUsedCertificate';");
    q.exec();

    q.prepare("DELETE FROM globals WHERE `name`='shopCashRegisterId';");
    q.exec();

    q.prepare("DELETE FROM globals WHERE `name`='signatureModuleIsDamaged';");
    q.exec();

    q.prepare("UPDATE globals SET value = 0 WHERE name = 'turnovercounter';");
    q.exec();

    q.prepare("UPDATE globals SET value = 0 WHERE name = 'DEP';");
    q.exec();

    q.prepare("DELETE FROM globals WHERE `name`='CASHREGISTER INAKTIV';");
    q.exec();

    QString dbType = getDatabaseType();

    if (dbType == "QMYSQL") {
        /*MYSQL*/
        q.prepare("ALTER TABLE journal AUTO_INCREMENT = 1;");
        q.exec();

        q.prepare("ALTER TABLE orders AUTO_INCREMENT = 1;");
        q.exec();

        q.prepare("ALTER TABLE receipts AUTO_INCREMENT = 1;");
        q.exec();

        q.prepare("ALTER TABLE dep AUTO_INCREMENT = 1;");
        q.exec();
    } else {
        /*SQLITE*/
        q.prepare("delete from sqlite_sequence where name='journal';");
        q.exec();

        q.prepare("delete from sqlite_sequence where name='orders';");
        q.exec();

        q.prepare("delete from sqlite_sequence where name='receipts';");
        q.exec();

        q.prepare("delete from sqlite_sequence where name='dep';");
        q.exec();
    }

    q.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,text) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, 'Id\tProgrammversion\tKassen-Id\tKonfigurationsänderung\tBeschreibung\tErstellungsdatum')"));
    q.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,text) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, 'Id\tProgrammversion\tKassen-Id\tProduktposition\tBeschreibung\tMenge\tEinzelpreis\tGesamtpreis\tUSt. Satz\tErstellungsdatum')"));
    q.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,text) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, 'Id\tProgrammversion\tKassen-Id\tBeleg\tBelegtyp\tBemerkung\tNachbonierung\tBelegnummer\tDatum\tUmsatz Normal\tUmsatz Ermaessigt1\tUmsatz Ermaessigt2\tUmsatz Null\tUmsatz Besonders\tJahresumsatz bisher\tErstellungsdatum')"));
    q.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,text) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, 'Id\tProgrammversion\tKassen-Id\tBeleg-Textposition\tText\tErstellungsdatum')"));
}

void Database::cleanup()
{
    Database::updateGlobals("defaulttax", NULL, "20");
    Database::updateGlobals("CASHREGISTER INAKTIV", "0", NULL);
}

QString Database::updateGlobals(QString name, QString defaultvalue, QString defaultStrValue)
{
    if (!defaultStrValue.isEmpty())
        globalStringValues.insert(name, defaultStrValue);

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QSqlQuery queryDelete(dbc);

    bool insertnew = true;

    query.prepare("SELECT id, value, strValue FROM globals WHERE name=:name");
    query.bindValue(":name", name);
    query.exec();
    if (query.next()) {
        defaultvalue = query.value("value").toString().isNull() ? NULL : query.value("value").toString();
        defaultStrValue = query.value("strValue").toString().isNull() ? NULL : query.value("strValue").toString();
        insertnew = false;
    }

    queryDelete.prepare(QString("DELETE FROM globals WHERE id=:id"));
    while (query.next()) {
        queryDelete.bindValue(":id", query.value("id").toInt());
        queryDelete.exec();
    }

    if (insertnew) {
        query.prepare("INSERT INTO globals (name, value, strValue) VALUES (:name, :value, :strValue)");
        query.bindValue(":name", name);
        if (!defaultvalue.isNull()) query.bindValue(":value", defaultvalue.toInt());
        if (!defaultStrValue.isNull()) query.bindValue(":strValue", defaultStrValue);
        query.exec();
    }

    return defaultvalue.isNull() ? defaultStrValue : defaultvalue;
}

QSqlDatabase Database::database()
{
    QSqlDatabase dbc = DatabaseManager::database("CN");
    if (!dbc.lastError().nativeErrorCode().isEmpty())
        qDebug() << "Function Name: " << Q_FUNC_INFO << dbc.lastError().text() << " #" << dbc.lastError().nativeErrorCode();
    return dbc;
}

bool Database::isAnyValueFunctionAvailable()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    return query.exec("SELECT ANY_VALUE(value) FROM globals");
}

QString Database::getDatabaseVersion()
{

    if (globalStringValues.contains("databasetype"))
        return globalStringValues.value("databasetype");

    QString dbType = getDatabaseType();

    if (dbType == "QSQLITE") {
        QSqlDatabase dbc = Database::database();
        QSqlQuery query(dbc);
        query.exec("SELECT sqlite_version()");
        if (query.next())
            dbType += " " + query.value(0).toString();

        dbType += " / " + QFileInfo(dbc.databaseName()).baseName();
        globalStringValues.insert("databasetype", dbType);

    } else if (dbType == "QMYSQL") {
        QSqlDatabase dbc = Database::database();
        QSqlQuery query(dbc);
        query.exec("SHOW VARIABLES LIKE 'version'");
        if (query.next())
            dbType += " " + query.value(1).toString();

        dbType += " / " + dbc.hostName() + " / " + dbc.databaseName();
        globalStringValues.insert("databasetype", dbType);
    }

    globalStringValues.insert("databasetype", dbType);
    return dbType;
}
