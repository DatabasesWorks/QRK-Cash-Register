/*
 * This file is part of QRK - Qt Registrier Kasse
 *
 * Copyright (C) 2015-2019 Christian Kvasny <chris@ckvsoft.at>
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
#include "utils/utils.h"
#include "preferences/qrksettings.h"
#include "databasemanager.h"
#include "journal.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "backup.h"

#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QDate>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

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

QString Database::getLastExecutedQuery(const QSqlQuery &query)
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

QMap<QString, QMap<QString, double> > Database::getSalesPerUser(const QString &from, const QString &to, int &size)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    /* Umsätze Zahlungsmittel */
    query.prepare("SELECT actionTypes.actionText, receiptNum, gross, users.username, r2b from receipts LEFT JOIN actionTypes on receipts.payedBy=actionTypes.actionId LEFT JOIN users ON receipts.userId=users.ID WHERE receipts.timestamp between :fromDate AND :toDate AND receipts.payedBy < 3 ORDER BY users.username, receipts.payedBy");

    query.bindValue(":fromDate", from);
    query.bindValue(":toDate", to);
    query.exec();

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    /*
     * FIXME: We do this workaroud, while SUM and ROUND will
     * give use the false result. SQL ROUND/SUM give xx.98 from xx.985
     * should be xx.99
     */

    QMap<QString, double> zm;
    QMap<QString, QMap<QString, double> > user;
    while (query.next()) {
        int r2b = query.value("r2b").toInt();
        QString username = query.value("username").toString();
        QString key = query.value("actionText").toString();
        if (r2b == 1)
            key += " R2B";
        int id = query.value("receiptNum").toInt();
        QBCMath total(query.value("gross").toString());
        total.round(2);

        QMap<int, double> mixed = Database::getGiven(id);
        if (mixed.size() > 1) {
            QString type = Database::getActionType(mixed.lastKey());
            if (r2b == 1)
                type += " R2B";

            QBCMath secondPay(mixed.last());
            secondPay.round(2);
            total -= secondPay;
            if ( zm.contains(type) ) {
                zm[type] += secondPay.toDouble();
            } else {
                zm[type] = secondPay.toDouble();
            }
        }

        if ( zm.contains(key) ) {
            zm[key] += total.toDouble();
        } else {
            zm[key] = total.toDouble();
        }

        if (user.contains(username)) {
            QMap<QString, double> zm2 = user[username];
            QMap<QString, double>::iterator i;
            for (i = zm.begin(); i != zm.end(); ++i) {
                QString key = i.key();
                if (zm2.contains(key)) {
                    zm2[key] += i.value();
                } else {
                    zm2[key] = i.value();
                }
            }
            user[username] = zm2;
        } else {
            user[username] = zm;
        }
        zm.clear();
    }
    size = zm.size();
    return user;
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

void Database::updateSortorder(const QString &which, QList<int> indexList)
{
    if (which.isEmpty())
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    dbc.transaction();
    query.prepare(QString("UPDATE %1 SET sortorder=:sortorder WHERE id=:id").arg(which));

    bool ok = false;
    for (int i = 0; i < indexList.count(); i++) {
        query.bindValue(":sortorder", i);
        query.bindValue(":id", indexList.at(i));
        ok = query.exec();
        if (!ok) {
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
            break;
        }
    }
    if (ok)
        dbc.commit();
    else
        dbc.rollback();
}

void Database::updateProductSold(double count, const QString &product)
{
    if (product.startsWith("Zahlungsbeleg für Rechnung"))
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    int id = getProductIdByName(product);

    query.prepare(QString("UPDATE products SET sold=sold+:sold, stock=stock-:stock WHERE id=:id"));
    query.bindValue(":sold", count);
    query.bindValue(":stock", count);
    query.bindValue(":id", id);

    query.exec();

}

void Database::updateProductPrice(double gross, const QString &product)
{
    if (product.startsWith("Zahlungsbeleg für Rechnung"))
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    int id = getProductIdByName(product);

    query.prepare(QString("UPDATE products SET gross=:gross, lastchange=:lastchange WHERE id=:id"));
    query.bindValue(":gross", gross);
    query.bindValue(":lastchange", QDateTime::currentDateTime());
    query.bindValue(":id", id);

    query.exec();
}

void Database::updateProductTax(double tax, const QString &product)
{
    if (product.startsWith("Zahlungsbeleg für Rechnung"))
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    int id = getProductIdByName(product);

    query.prepare(QString("UPDATE products SET tax=:tax, lastchange=:lastchange WHERE id=:id"));
    query.bindValue(":tax", tax);
    query.bindValue(":lastchange", QDateTime::currentDateTime());
    query.bindValue(":id", id);

    query.exec();
}

void Database::insertProductItemnumToExistingProduct(const QString &itemnum, const QString &product)
{
    if (product.startsWith("Zahlungsbeleg für Rechnung"))
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    int id = getProductIdByName(product);

    query.prepare(QString("UPDATE products SET itemnum=:itemnum WHERE id=:id"));
    query.bindValue(":itemnum", itemnum);
    query.bindValue(":id", id);

    query.exec();
}

bool Database::moveProductsToDefaultGroup(int oldgroupid)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("UPDATE products SET groupid=2 WHERE groupid=:id"));
    query.bindValue(":id", oldgroupid);

    return query.exec();
}

bool Database::moveGroupsToDefaultCategory(int oldcategoryid)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("UPDATE groups SET categoryId=1 WHERE categoryId=:id"));
    query.bindValue(":id", oldcategoryid);

    return query.exec();
}

QStringList Database::getStockInfoList()
{
    QrkSettings settings;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("select name, stock, minstock from products inner join orders on products.id=orders.product where orders.receiptId= (select max(receipts.receiptNum) from receipts) and products.stock <= products.minstock");
    query.exec();

    int decimals = settings.value("decimalDigits", 2).toInt();
    QStringList list;
    QBCMath stock;
    QBCMath minstock;
    QString name;
    while(query.next()) {
        name = query.value("name").toString();
        if (name.startsWith("Zahlungsbeleg für Rechnung"))
            continue;

        stock = query.value("stock").toString();
        minstock = query.value("minstock").toString();

        list.append(QString("%1 (%2 / %3)").arg(query.value("name").toString())
                    .arg(QBCMath::bcround(stock.toString(), decimals))
                    .arg(QBCMath::bcround(minstock.toString(), decimals)));
    }
    return list;
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

    return Database::updateGlobals("taxlocation", Q_NULLPTR, "AT");
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

    return Database::updateGlobals("defaulttax", Q_NULLPTR, "20");
}

QString Database::getAdvertisingText()
{
    // AdvertisingText
    if (globalStringValues.contains("printAdvertisingText"))
        return globalStringValues.value("printAdvertisingText");

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='printAdvertisingText'");
    query.exec();
    if (query.next())
        return query.value(0).toString();

    return Database::updateGlobals("printAdvertisingText", Q_NULLPTR, "");
}

QString Database::getHeaderText()
{
    // Header
    if (globalStringValues.contains("printHeader"))
        return globalStringValues.value("printHeader");

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='printHeader'");
    query.exec();
    if (query.next())
        return query.value(0).toString();

    return Database::updateGlobals("printHeader", Q_NULLPTR, "");
}

QString Database::getFooterText()
{
    // Footer
    if (globalStringValues.contains("printFooter"))
        return globalStringValues.value("printFooter");

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='printFooter'");
    query.exec();
    if (query.next())
        return query.value(0).toString();

    return Database::updateGlobals("printFooter", Q_NULLPTR, "");
}

QJsonArray Database::getPrinters()
{
    QJsonArray printers;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT id, name, printer, definition FROM printers");
    query.exec();
    while (query.next()) {
        QJsonObject printer;
        printer["name"] = query.value("name").toString();
        printer["id"] = query.value("id").toInt();
        printer["printer"] = query.value("printer").toString();
        printer["definition"] = query.value("definition").toJsonArray();
        printers.append(printer);
    }

    return printers;
}

QString Database::getPrinterName(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT name FROM printers WHERE id=:id");
    query.bindValue(":id", id);
    query.exec();
    if (query.next()) {
        return query.value("name").toString();
    }

    return "QrkPDF";
}

int Database::getPrinterIdFromProduct(int id)
{
    int printerid = -1;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QSqlQuery query2(dbc);
    QSqlQuery query3(dbc);
    query.prepare(QString("SELECT printerid, groupid FROM products WHERE id=:id"));
    query.bindValue(":id", id);
    query.exec();
    if (query.next()) {
        if (query.value("printerid").isNull()) {
            query2.prepare(QString("SELECT printerid, categoryId FROM groups WHERE id=:id"));
            query2.bindValue(":id", query.value("groupid").toInt());
            query2.exec();
            if (query2.next()) {
                if (query2.value("printerid").isNull()) {
                    query3.prepare(QString("SELECT printerid FROM categories WHERE id=:id"));
                    query3.bindValue(":id", query2.value("categoryId").toInt());
                    query3.exec();
                    if (query3.next()) {
                        if (query3.value("printerid").isNull()) {
                            return printerid;
                        } else {
                            return query3.value("printerid").toInt();
                        }
                    }
                } else {
                    return query2.value("printerid").toInt();
                }
            }
        } else {
            return query.value("printerid").toInt();
        }
    }
    return printerid;
}

QJsonArray Database::getDefinitions()
{
    QJsonArray definitions;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT id, name FROM printerdefs");
    query.exec();
    while (query.next()) {
        QJsonObject definition;
        definition["name"] = query.value("name").toString();
        definition["id"] = query.value("id").toString();
        definitions.append(definition);
    }

    return definitions;
}

int Database::getDefinitionId(const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    int id = 0;
    query.prepare("SELECT id FROM printerdefs WHERE name=:name");
    query.bindValue(":name", name);
    query.exec();
    while (query.next()) {
        id = query.value("id").toInt();
    }

    return id;
}

QString Database::getDefinitionName(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString name = "n/a";
    query.prepare("SELECT name FROM printerdefs WHERE id=:id");
    query.bindValue(":id", id);
    query.exec();
    while (query.next()) {
        name = query.value("name").toString();
    }

    return name;
}

//--------------------------------------------------------------------------------

QString Database::getShortCurrency()
{
    if (globalStringValues.contains("shortcurrency"))
        return globalStringValues.value("shortcurrency");

    QString currency = getCurrency();
    QString currencySymbol;

    if (currency == "CHF")
        currencySymbol = "Fr";
    else
        currencySymbol = "€";

    Database::updateGlobals("shortcurrency", Q_NULLPTR, currencySymbol);
    return currencySymbol;
}
//--------------------------------------------------------------------------------

QString Database::getCurrency()
{
    if (globalStringValues.contains("currency"))
        return globalStringValues.value("currency");

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='currency'");
    query.exec();
    if (query.next()) {
        globalStringValues.insert("currency", query.value(0).toString());
    } else {
        return Database::updateGlobals("currency", Q_NULLPTR, QLocale().currencySymbol());
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

        return id;
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

bool Database::addCustomerText(int id, const QString &text)
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

int Database::getProductIdByName(const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    bool ok = query.prepare("select p2.id from (select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where name=:name");
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

int Database::getProductIdByNumber(const QString &number)
{
    if (number.isEmpty())
        return -1;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    bool ok = query.prepare("select p2.id from (select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where itemnum=:number");
    query.bindValue(":number", number);

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

int Database::getProductIdByBarcode(const QString &code)
{
    if (code.isEmpty())
        return -1;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    bool ok = query.prepare("select p2.id from (select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where barcode=:barcode");
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

QString Database::getProductNameById(int id)
{
    if (id == 0)
        return QString();

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString q = QString("SELECT name FROM products WHERE id=:id");
    bool ok = query.prepare(q);
    query.bindValue(":id", id);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
    }

    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
    }
    if (query.next()) {
        return query.value("name").toString();
    }

    return QString();
}

bool Database::addProduct(const QJsonObject &data)
{
    bool productExists = Database::exists(data["name"].toString());

    int visible = data["visible"].toInt();
    int group = 2;
    QString itemNum = "";

    if (!data["itemnum"].toString().isEmpty()) {
        if (Utils::isNumber(data["itemnum"].toString())) {
            itemNum = (data["itemnum"].toString().toInt() == 0)?Database::getNextProductNumber():data["itemnum"].toString();
        } else {
            itemNum = data["itemnum"].toString();
        }
    } else {
        itemNum = Database::getNextProductNumber();
    }

    if (!data["group"].toString().isNull())
        group = data["group"].toInt();
    if (data["name"].toString().startsWith("Zahlungsbeleg für Rechnung") || data["name"].toString().startsWith("Startbeleg")) {
        if (productExists) {
            if (data["name"].toString().startsWith("Startbeleg")) {
                qWarning() << "Function Name: " << Q_FUNC_INFO << "Ein Startbeleg ist schon vorhanden.";
                return true;
            }
            return false;
        }

        /* We do not need to display this in the Manager*/
        visible = 0;
        group = 1;
        itemNum = "";
    }

    if (data["name"].toString().startsWith("Monatsbeleg") || data["name"].toString().startsWith("Jahresbeleg")) {
        itemNum = "";
        group = 1;
    }

    if (productExists) {
        if ((data["itemnum"].toString().toInt() == 0) && !itemNum.isEmpty() && itemNum != "D") {
            Database::insertProductItemnumToExistingProduct(itemNum, data["name"].toString());
        }
        return true;
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString q = QString("INSERT INTO products (name, itemnum, barcode, tax, net, gross, visible, groupid, version, origin) VALUES (:name, :itemnum, '', :tax, :net, :gross, :visible, :group, :version, :origin)");
    bool ok = query.prepare(q);

    query.bindValue(":name", data["name"].toString());
    query.bindValue(":itemnum", itemNum);
    query.bindValue(":tax", data["tax"].toDouble());
    query.bindValue(":net", data["net"].toDouble());
    query.bindValue(":gross", data["gross"].toDouble());
    query.bindValue(":visible", visible);
    query.bindValue(":group", group);
    query.bindValue(":version", 0);
    query.bindValue(":origin", -1);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
        return false;
    }

    if (query.exec()) {
        int id = Database::getProductIdByName(data["name"].toString());
        if (id > 0) {
            query.prepare(QString("UPDATE products SET origin=:origin WHERE id=:id"));
            query.bindValue(":id", id);
            query.bindValue(":origin", id);
            ok = query.exec();
            if (!ok) {
                qWarning() << "Function Name: " << Q_FUNC_INFO << " error: " << query.lastError().text();
                qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
            }
            return ok;
        }
        return true;
    } else {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
    }

    return false;
}

QJsonObject Database::getProductByName(const QString &name, int visible)
{
    QJsonObject data;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    bool ok = query.prepare("select p2.name, p2.itemnum, p2.barcode, p2.tax, p2.net, p2.gross, p2.description, p2.version, p2.origin from (select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin WHERE name=:name AND visible >= :visible");

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
        return QJsonObject();
    }

    query.bindValue(":name", name);
    query.bindValue(":visible", visible);

    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
        return QJsonObject();
    }

    if (query.next()) {
        data["name"] = query.value("name").toString().trimmed();
        data["itemnum"] = query.value("itemnum").toString().trimmed();
        data["barcode"] = query.value("barcode").toString().trimmed();
        data["tax"] = query.value("tax").toDouble();
        data["net"] = query.value("net").toDouble();
        data["gross"] = query.value("gross").toDouble();
        data["description"] = query.value("description").toString().trimmed();
        data["version"] = query.value("version").toInt();
        data["origin"] = query.value("gross").toInt();
        return data;
    }

    return QJsonObject();
}

QJsonObject Database::getProductById(int id, int visible)
{
    if (id < 1)
        return QJsonObject();

    QJsonObject data;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    bool ok = query.prepare("select name, itemnum, barcode, tax, net, gross, description, version, origin from products WHERE id=:id AND visible >= :visible");

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
        return QJsonObject();
    }

    query.bindValue(":id", id);
    query.bindValue(":visible", visible);

    if (!query.exec()) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << getLastExecutedQuery(query);
        return QJsonObject();
    }

    if (query.next()) {
        data["name"] = query.value("name").toString().trimmed();
        data["itemnum"] = query.value("itemnum").toString().trimmed();
        data["barcode"] = query.value("barcode").toString().trimmed();
        data["tax"] = query.value("tax").toDouble();
        data["net"] = query.value("net").toDouble();
        data["gross"] = query.value("gross").toDouble();
        data["description"] = query.value("description").toString().trimmed();
        data["version"] = query.value("version").toInt();
        data["origin"] = query.value("origin").toInt();
        return data;
    }

    return QJsonObject();
}

qulonglong Database::getFirstProductNumber()
{

    if (globalStringValues.contains("firstproductnumber"))
        return globalStringValues.value("firstproductnumber").toULongLong();

    QrkSettings settings;
    qulonglong value = settings.value("firstProductnumber", 1).toULongLong();
    globalStringValues.insert("firstproductnumber", QString::number(value));
    return globalStringValues.value("firstproductnumber").toULongLong();
}

QString Database::getNextProductNumber(bool update)
{
    qulonglong value = getFirstProductNumber();

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString q = "SELECT MAX(CAST(itemnum as decimal)) AS lastItemNum FROM products";
    query.exec(q);
    if (query.next()) {
        QString itemnum = query.value("lastItemNum").toString();
        qulonglong maxItemnum = itemnum.toULongLong();
        maxItemnum = (value > maxItemnum)?value + 1: maxItemnum + 1;
        if (update)
            globalStringValues.insert("firstproductnumber", QString::number(maxItemnum));
        return QString::number(maxItemnum);
    }

    return "";
}

//--------------------------------------------------------------------------------

bool Database::exists(const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("select p2.id from (select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin WHERE name=:name");
    query.bindValue(":name", name);

    query.exec();
    if (query.next())
        return true;

    return false;
}

bool Database::exists(const QString &type, const int &id, const QString &fieldname)
{

    if (id == 0)
        return true;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT id FROM %1 WHERE %2=:id").arg(type).arg(fieldname));
    query.bindValue(":id", id);

    query.exec();
    if (query.next())
        return true;

    return false;
}

bool Database::exists(const QString &type, const QString &name, const QString &fieldname)
{

    if (name.isEmpty())
        return true;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT id FROM %1 WHERE %2=:name").arg(type).arg(fieldname));
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

void Database::setCurfewTime(QTime time)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString curfew = "curFew";

    QString timeString = time.toString("hh:mm:ss");
    query.prepare("UPDATE globals SET strValue=:time WHERE name=:curFew");
    query.bindValue(":curFew", curfew);
    query.bindValue(":time", timeString);
    if (query.exec()) {
        Database::updateGlobals(curfew, Q_NULLPTR, timeString);
    } else {
        query.prepare("INSERT INTO globals (name, strValue) VALUES(:curFew, :time)");
        query.bindValue(":curFew", curfew);
        query.bindValue(":time", timeString);
        if (query.exec())
            Database::updateGlobals(curfew, Q_NULLPTR, timeString);
    }
}

QTime Database::getCurfewTime()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QString curfew = "curFew";

    if (globalStringValues.contains("curfewTemp")) {
        return QTime::fromString(globalStringValues.value("curfewTemp"), "hh:mm:ss");
    }

    if (globalStringValues.contains(curfew)) {
        return QTime::fromString(globalStringValues.value(curfew), "hh:mm:ss");
    }

    query.prepare("SELECT strValue FROM globals WHERE name=:curFew");
    query.bindValue(":curFew", curfew);
    query.exec();
    if (query.next()) {
        QString time = query.value(0).toString();
        Database::updateGlobals(curfew, Q_NULLPTR, time);
        return QTime::fromString(globalStringValues.value(curfew), "hh:mm:ss");
    }

    Database::setCurfewTime(QTime(0,0,0));
    return QTime(0,0,0);
}

QTime Database::getLastEOACurfewTime()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT curfew FROM reports WHERE id=(SELECT max(id) FROM reports)");
    query.exec();
    if (query.next()) {
        QString time = query.value("curfew").toString();
        return QTime::fromString(time, "hh:mm");
    }

    return QTime(0,0,0);
}


QDate Database::getFirstReceiptDate()
{
    return getFirstReceiptDateTime().date();
}

QDateTime Database::getFirstReceiptDateTime()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT infodate FROM receipts where receiptNum IN (SELECT min(receiptNum) FROM receipts)");
    query.exec();
    QDateTime dt = QDateTime::currentDateTime();
    if (query.next()) {
        dt = query.value(0).toDateTime();
        return dt;
    }

    return QDateTime();
}

QDate Database::getLastReceiptDate()
{
    return getLastReceiptDateTime().date();
}

//--------------------------------------------------------------------------------

QDateTime Database::getLastReceiptDateTime(bool realtime)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    if(realtime)
        query.prepare("SELECT timestamp FROM receipts where receiptNum IN (SELECT max(receiptNum) FROM receipts)");
    else
        query.prepare("SELECT infodate FROM receipts where receiptNum IN (SELECT max(receiptNum) FROM receipts)");
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

    if (globalStringValues.contains("DB_type"))
        return globalStringValues.value("DB_type");

    // read global defintions (DB, ...)
    QrkSettings settings;

    return settings.value("DB_type").toString();
}

QStringList Database::getDatabaseTableHeaderNames(const QString &tablename)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT * FROM %1 LIMIT 1").arg(tablename));

    if (query.exec()) {
        QStringList list;
        if (query.next()) {
            const QSqlRecord record= query.record();
            int count = record.count();
            for(int i = 0; i < count; ++i) {
                list << tablename + "." + record.fieldName(i); //Headers
            }
            return list;
        }
    }

    return QStringList();
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
    QString tmp = "";
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    if (globalStringValues.contains("shopOwner")) {
        tmp = globalStringValues.value("shopOwner");
    } else {
        query.prepare("SELECT strValue FROM globals WHERE name='shopOwner'");
        query.exec();
        if (query.next())
            tmp = query.value(0).toString();

        Database::updateGlobals("shopOwner", Q_NULLPTR, tmp);
    }
    name = (tmp.isEmpty()) ? "" : "\n" + tmp;
    tmp = "";

    if (globalStringValues.contains("shopAddress")) {
        tmp = globalStringValues.value("shopAddress");
    } else {
        query.prepare("SELECT strValue FROM globals WHERE name='shopAddress'");
        query.exec();
        if (query.next())
            tmp = query.value(0).toString();

        Database::updateGlobals("shopAddress", Q_NULLPTR, tmp);
    }
    name += (tmp.isEmpty()) ? "" : "\n" + tmp;
    tmp = "";

    if (globalStringValues.contains("shopUid")) {
        tmp = globalStringValues.value("shopUid");
    } else {
        query.prepare("SELECT strValue FROM globals WHERE name='shopUid'");
        query.exec();
        if (query.next())
            tmp = query.value(0).toString();

        Database::updateGlobals("shopUid", Q_NULLPTR, tmp);
    }
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

QJsonObject Database::getConnectionDefinition()
{
    QJsonObject jobj;
    jobj.insert("dbtype", getDatabaseType());
    jobj.insert("databasename", globalStringValues.value("databasename"));
    jobj.insert("databasehost", globalStringValues.value("databasehost"));
    jobj.insert("databaseusername", globalStringValues.value("databaseusername"));
    jobj.insert("databasepassword", globalStringValues.value("databasepassword"));
    jobj.insert("databaseoptions", globalStringValues.value("databaseoptions"));
    return jobj;
}

bool Database::open(bool dbSelect, QString TESTFILE)
{
    const int CURRENT_SCHEMA_VERSION = 22;
    // read global defintions (DB, ...)
    QrkSettings settings;
    QJsonObject ConnectionDefinition = Database::getConnectionDefinition();

    QString dbType = ConnectionDefinition["dbtype"].toString();

    if (dbType.isEmpty() || dbSelect) {
        DatabaseDefinition dialog(Q_NULLPTR);

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

    QString connectionName = "";
    {
        QSqlDatabase currentConnection;

        if (!TESTFILE.isEmpty()) {
            dbType = "QSQLITE";
            globalStringValues.insert("databasename", TESTFILE);
            globalStringValues.insert("DB_type", "QSQLITE");
            currentConnection = QSqlDatabase::addDatabase("QSQLITE", "CN");
            currentConnection.setDatabaseName(TESTFILE);
        }
        // setup database connection
        else if (dbType == "QSQLITE") {
            globalStringValues.insert("databasename", QString(dataDir + "/%1-%2.db").arg(date.year()).arg(basename));
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
            globalStringValues.insert("databasehost", hostName);
            globalStringValues.insert("databaseusername", userName);
            globalStringValues.insert("databasepassword", password);
            globalStringValues.insert("databaseoptions", "MYSQL_OPT_RECONNECT=1;MYSQL_OPT_CONNECT_TIMEOUT=86400;MYSQL_OPT_READ_TIMEOUT=60");
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
            globalStringValues.insert("databasename", basename);
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

            {
                QSqlQuery query(currentConnection);
                query.exec("PRAGMA integrity_check;");
                query.next();
                QString result = query.value(0).toString();
                lastError = query.lastError().text();
                if (result != "ok") {
                    QMessageBox errorDialog;
                    errorDialog.setIcon(QMessageBox::Critical);
                    errorDialog.addButton(QMessageBox::Ok);
                    errorDialog.setText(lastError + ".\n" + tr("Die SQL-Datenbank wurde beschädigt. Stellen Sie diese von einen aktuellen Backup wieder her."));
                    errorDialog.setWindowTitle(QObject::tr("Datenbank Fehler"));
                    errorDialog.exec();
                    return false;
                }
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

            query.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,data,checksum) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, '9f11c3693ee2f40c9c10d741bc13f5652586fa564a071eb231541abf64dc1f5aa4c8845119ddc2734e836dfa394426d02609a90ad99d5ec1212988424e16c9f47f679b48253b55b0af91d0ee22dacc9f947201288d48b7f14a6fe1c895e1b4cf','DBD7ACB39B653D948DEDD342B912D66F14482DBB')"));
            query.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,data,checksum) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, '9f11c3693ee2f40c9c10d741bc13f5652782f7d13ee7f6bd1725691470a0ced96ea4ef910accfa7f416797b7a73c17f239a1abe7f52887d582a719a320d480b76e90b95edfe7eda059aca296e2916d6fa0cfee77d4db0dd01a25d3720f89f633e314241be48b6078a3dc4a13fb11cea51a9c582dff0b7dae944945f9d84eb72a','08EF99A8218FA5130A2B0C58F27D897195162744')"));
            query.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,data,checksum) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, '9f11c3693ee2f40c9c10d741bc13f5656545e1d75d25e2b35c2b958d9c37bc733b080292d1738c51366c0cc0e5317acdae667e3e1a166ba75be259466db75a5546ab362107a37cb7a3bd2eaf5785c3d7baa9d5aa723cb1b8333a66af9e59dfced2cbc6f233614ac425b77794d0541ab86019388693a8d32064cfadddaff462412bfd20a876c8bbf503bf224561fe52b2551258978ed8bd0d33382a5d3c5b9768f2828b512d3264dd6b8c1314b0a15856b3ae5f8ebe5830fdd5e2629c18fc2b3e8511eaa6aaf59fc359a531fb6e5f8658','5C29C8A36F5B46F46CE78FB4502F4DC8DFDCEBA6')"));
            query.exec(QString("INSERT INTO `journal`(id,version,cashregisterid,datetime,data,checksum) VALUES (NULL,'0.15.1222',0,CURRENT_TIMESTAMP, '9f11c3693ee2f40c9c10d741bc13f5656545e1d75d25e2b35c2b958d9c37bc73a79a1c487e6ea70eb7e63c6e708d983879852e1a8d9ea66167824c5312f1d12ca86ae59bd4498a5f6b4cecfd27e28218','6B4D651268E7436F43E668590BFC5D6C86F1AEE7')"));
            updatePrinters();
            query.exec(QString("UPDATE `categories` SET printerid=NULL"));
            query.exec(QString("UPDATE `groups` SET printerid=NULL"));
            query.exec(QString("UPDATE `products` SET printerid=NULL"));

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

            if (schemaVersion < CURRENT_SCHEMA_VERSION)
                Backup::create();

            // run all db update scripts from the db version + 1 to what the program currently needs
            for (int i = schemaVersion + 1; i <= CURRENT_SCHEMA_VERSION; i++) {
                QFile f;

                if (dbType == "QSQLITE")
                    f.setFileName(QString(":src/sql/QRK-sqlite-update-%1.sql").arg(i));
                else if (dbType == "QMYSQL")
                    f.setFileName(QString(":src/sql/QRK-mysql-update-%1.sql").arg(i));

                if (!f.open(QIODevice::ReadOnly)) {
                    qCritical("could not load internal update file %s", qPrintable(f.fileName()));
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
                        qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                        qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
                        currentConnection.rollback();
                        return false;
                    }
                }

                // changes which are not possible in sql file needed for update-7
                if (i == 7) {
                    ok = query.exec(QString("UPDATE products SET groupid=2 WHERE name NOT LIKE 'Zahlungsbeleg für Rechnung%'"));
                    if (!ok) {
                        qWarning() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                        qWarning() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
                    }
                    ok = query.exec(QString("UPDATE products SET groupid=1 WHERE name LIKE 'Zahlungsbeleg für Rechnung%'"));
                    if (!ok) {
                        qWarning() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                        qWarning() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
                    }
                }
                if (i == 14) {
                    ok = query.exec(QString("UPDATE journal SET `text`='%1' WHERE id=1").arg("Id\tProgrammversion\tKassen-Id\tKonfigurationsänderung\tBeschreibung\tErstellungsdatum"));
                    if (!ok) {
                        qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
                        qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
                        currentConnection.rollback();
                        return false;
                    }
                }
                if (i == 19) {
                    Journal::encodeJournal(currentConnection);
                }
                if (i == 21) {
                    updatePrinters();
                    fixDoubleProductNames();
                    query.exec("UPDATE `categories` SET printerid=NULL");
                    query.exec("UPDATE `groups` SET printerid=NULL");
                    query.exec("UPDATE `products` SET printerid=NULL");
                    query.exec("update products set itemnum='' where itemnum in (select itemnum from ((select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin) where itemnum !='' group by itemnum having count(itemnum) > 1)");
                }

                query.exec(QString("UPDATE globals SET value=%1 WHERE name='schemaVersion'").arg(i));
            }

            if (schemaVersion != CURRENT_SCHEMA_VERSION)
                query.exec(QString("UPDATE globals SET value=%1 WHERE name='schemaVersion'")
                           .arg(CURRENT_SCHEMA_VERSION));
        }

        if (dbType == "QSQLITE") {
            // enforce foreign key constraint
            query.exec("PRAGMA foreign_keys = 1;");
            query.exec("PRAGMA journal_mode;");
            query.next();
            QString mode = query.value(0).toString();
            if (mode != "wal") {
                query.exec("PRAGMA journal_mode = WAL;");
                qDebug() << "Function Name: " << Q_FUNC_INFO << "change SQLite mode from " << mode << " to \"wal\"";
            }
        }

        // check for group with id 1 and id 2
        // these are absolutely necessary
        query.exec("SELECT id FROM groups WHERE id=1");
        if (!query.next()) {
            query.exec("INSERT INTO `groups`(id,name,visible) VALUES (1,'auto',0);");
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Database manibulation: missing groupid -> 1";
        }
        query.exec("SELECT id FROM groups WHERE id=2");
        if (!query.next()) {
            query.exec("INSERT INTO `groups`(id,name,visible) VALUES (2,'Standard',0);");
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Database manibulation: missing groupid -> 2";
        }

        query.exec("SELECT id FROM categories WHERE id=1");
        if (!query.next()) {
            query.exec("INSERT INTO `categories`(id,name) VALUES (1,'Standard');");
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Database manibulation: missing categoryid -> 1";
        }

        connectionName = currentConnection.connectionName();
        currentConnection.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
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

QMap<int, double> Database::getGiven(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    QMap<int, double> given;
    query.prepare(QString("SELECT payedBy, gross FROM receiptspay WHERE receiptNum=%1").arg(id));
    query.exec();
    while (query.next()) {
        given.insert(query.value("payedBy").toInt(), query.value("gross").toDouble());
    }

    return given;

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

QString Database::getTaxType(double id)
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
            q.prepare("UPDATE globals set value=:value WHERE name=:name;");
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

    q.prepare("DELETE FROM products WHERE groupid=1;");
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
    Database::updateGlobals("defaulttax", Q_NULLPTR, "20");
    Database::updateGlobals("CASHREGISTER INAKTIV", "0", Q_NULLPTR);
}

void Database::fixDoubleProductNames()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QSqlQuery updatequery(dbc);

    query.exec("update products set itemnum=TRIM(itemnum), barcode=TRIM(barcode), name=TRIM(name)");

    query.prepare("select * from products where name in (select name from ((select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin) group by name having count(name) > 1) order by name, id");
    query.exec();

    updatequery.prepare("update products set version=:version, origin=:origin where id=:id");
    int productid = 0;
    int version = 0;
    int origin = 0;
    QString name = "";
    while (query.next()) {
        if (name != query.value("name").toString()) {
            name = query.value("name").toString();
            origin = query.value("id").toInt();
            version = query.value("version").toInt();
        } else {
            productid = query.value("id").toInt();
            version++;
            updatequery.bindValue(":origin", origin);
            updatequery.bindValue(":version", version);
            updatequery.bindValue(":id", productid);
            updatequery.exec();
        }
    }
}

void Database::updatePrinters()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    QrkSettings settings;
    QJsonObject obj, printer;

    obj["type"] = "custom";
    obj["pdf"] = false;
    obj["paperWidth"] = settings.value("paperWidth", 80).toInt();
    obj["paperHeight"] = settings.value("paperHeight", 210).toInt();
    obj["marginLeft"] = settings.value("marginLeft", 0).toDouble();
    obj["marginTop"] = settings.value("marginTop", 17).toDouble();
    obj["marginRight"] = settings.value("marginRight", 5).toDouble();
    obj["marginBottom"] = settings.value("marginBottom", 0).toDouble();
    QJsonDocument doc(obj);
    query.prepare("INSERT INTO printerdefs (name, definition) VALUES (:name, :definition)");
    query.bindValue(":name", "receipt");
    query.bindValue(":definition", doc.toBinaryData().toBase64());
    query.exec();
    query.exec("SELECT max(id) AS id FROM printerdefs");
    query.next();
    int id = query.value("id").toInt();
    printer["name"] = settings.value("receiptPrinter", "QrkPDF").toString();
    printer["definitionid"] = id;
    query.prepare("INSERT INTO printers (name, printer, definition) VALUES (:name, :printer, :definition)");
    query.bindValue(":name", "Bondrucker");
    query.bindValue(":printer", QJsonDocument(QJsonArray() << printer).toBinaryData().toBase64());
    query.bindValue(":definition", id);
    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
    query.exec("SELECT max(id) AS id FROM printers");
    query.next();
    int printerid = query.value("id").toInt();

    settings.beginGroup("Printer");
    settings.save2Settings("receiptPrinter", id);
    settings.endGroup();

    printer["name"] = settings.value("collectionPrinter", "QrkPDF").toString();;
    printer["definitionid"] = id;

    query.prepare("INSERT INTO printers (name, printer, definition) VALUES (:name, :printer, :definition)");
    query.bindValue(":name", "Abholbondrucker");
    query.bindValue(":printer", QJsonDocument(QJsonArray() << printer).toBinaryData().toBase64());
    query.bindValue(":definition", id);
    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
    query.exec("SELECT max(id) AS id FROM printers");
    query.next();
    printerid = query.value("id").toInt();

    settings.beginGroup("Printer");
    settings.save2Settings("collectionPrinter", printerid);
    settings.endGroup();

    obj.remove("paperWidth");
    obj.remove("paperHeight");
    obj["type"] = settings.value("invoiceCompanyPaperFormat", "A4").toString();
    obj["marginLeft"] = settings.value("invoiceCompanyMarginLeft", 90).toDouble();
    obj["marginTop"] = settings.value("invoiceCompanyMarginTop", 50).toDouble();
    obj["marginRight"] = settings.value("invoiceCompanyMarginRight", 10).toDouble();
    obj["marginBottom"] = settings.value("invoiceCompanyMarginBottom", 0).toDouble();
    doc.setObject(obj);
    query.prepare("INSERT INTO printerdefs (name, definition) VALUES (:name, :definition)");
    query.bindValue(":name", "invoice");
    query.bindValue(":definition", doc.toBinaryData().toBase64());
    query.exec();
    query.exec("SELECT max(id) AS id FROM printerdefs");
    query.next();
    id = query.value("id").toInt();

    printer["name"] = settings.value("invoiceCompanyPrinter", "QrkPDF").toString();
    printer["definitionid"] = id;

    query.prepare("INSERT INTO printers (name, printer, definition) VALUES (:name, :printer, :definition)");
    query.bindValue(":name", "Firmenrechnungsdrucker");
    query.bindValue(":printer", QJsonDocument(QJsonArray() << printer).toBinaryData().toBase64());
    query.bindValue(":definition", id);
    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
    query.exec("SELECT max(id) AS id FROM printers");
    query.next();
    printerid = query.value("id").toInt();

    settings.beginGroup("Printer");
    settings.save2Settings("invoiceCompanyPrinter", printerid);
    settings.endGroup();

    bool usePDF = settings.value("reportPrinterPDF", false).toBool();
    if (usePDF)
        obj["pdf"] = true;
    else
        obj["pdf"] = false;

    obj["type"] = settings.value("paperFormat", "A4").toString();

    obj["marginLeft"] = 5;
    obj["marginTop"] = 5;
    obj["marginRight"] = 5;
    obj["marginBottom"] = 5;
    doc.setObject(obj);
    query.prepare("INSERT INTO printerdefs (name, definition) VALUES (:name, :definition)");
    query.bindValue(":name", "reports");
    query.bindValue(":definition", doc.toBinaryData().toBase64());
    query.exec();
    query.exec("SELECT max(id) AS id FROM printerdefs");
    query.next();
    id = query.value("id").toInt();

    printer["name"] = settings.value("reportPrinter", "QrkPDF").toString();
    printer["definitionid"] = id;

    query.prepare("INSERT INTO printers (name, printer, definition) VALUES (:name, :printer, :definition)");
    query.bindValue(":name", "Berichtdrucker");
    query.bindValue(":printer", QJsonDocument(QJsonArray() << printer).toBinaryData().toBase64());
    query.bindValue(":definition", id);
    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
    query.exec("SELECT max(id) AS id FROM printers");
    query.next();
    printerid = query.value("id").toInt();

    bool noPrinter = settings.value("noPrinter").toBool();
    settings.beginGroup("Printer");
    settings.save2Settings("reportPrinter", printerid);
    settings.save2Settings("noPrinter", noPrinter);
    settings.endGroup();

    settings.removeSettings("receiptPrinter");

    settings.removeSettings("reportPrinter");
    settings.removeSettings("noPrinter");
    settings.removeSettings("numberCopies");

    settings.removeSettings("collectionPrinter");
    settings.removeSettings("collectionPrinterPaperFormat");
    settings.removeSettings("collectionReceiptCopies");

    settings.removeSettings("invoiceCompanyPrinter");
    settings.removeSettings("invoiceCompanyMarginBottom");
    settings.removeSettings("invoiceCompanyMarginLeft");
    settings.removeSettings("invoiceCompanyMarginRight");
    settings.removeSettings("invoiceCompanyMarginTop");
    settings.removeSettings("invoiceCompanyPaperFormat");

    settings.removeSettings("reportPrinterPDF");
    settings.removeSettings("paperFormat");
    settings.removeSettings("paperHeight");
    settings.removeSettings("paperWidth");
    settings.removeSettings("marginBottom");
    settings.removeSettings("marginLeft");
    settings.removeSettings("marginRight");
    settings.removeSettings("marginTop");
}

QString Database::updateGlobals(const QString &name, QString defaultvalue, QString defaultStrValue)
{
    if (defaultStrValue.isNull() && defaultvalue.isNull()) {
        globalStringValues.remove(name);
        return "";
    }

    if (!defaultStrValue.isNull())
        globalStringValues.insert(name, defaultStrValue);

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QSqlQuery queryDelete(dbc);

    bool insertnew = true;

    query.prepare("SELECT id, value, strValue FROM globals WHERE name=:name");
    query.bindValue(":name", name);
    query.exec();
    if (query.next()) {
        defaultvalue = query.value("value").toString().isNull() ? Q_NULLPTR : query.value("value").toString();
        defaultStrValue = query.value("strValue").toString().isNull() ? Q_NULLPTR : query.value("strValue").toString();
        insertnew = false;
        if (!defaultStrValue.isEmpty())
            globalStringValues.insert(name, defaultStrValue);
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

QSqlDatabase Database::database(const QString &connectionname)
{
    QSqlDatabase dbc = DatabaseManager::database(connectionname);
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

        query.exec("PRAGMA journal_mode;");
        query.next();
        QString mode = query.value(0).toString();

        query.exec("SELECT sqlite_version()");
        if (query.next())
            dbType += " " + query.value(0).toString();

        dbType += " / " + QFileInfo(dbc.databaseName()).baseName() + " / journalmode = " + mode;
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
