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

#include "defines.h"
#include "receiptitemmodel.h"
#include "database.h"
#include "utils/utils.h"
#include "documentprinter.h"
#include "journal.h"
#include "reports.h"
#include "RK/rk_signaturemodulefactory.h"
#include "utils/demomode.h"
#include "pluginmanager/pluginmanager.h"
#include "preferences/qrksettings.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "3rdparty/ckvsoft/rbac/acl.h"

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QDebug>

ReceiptItemModel::ReceiptItemModel(QObject* parent)
    : QStandardItemModel(parent)
{
    m_currency = "";
    m_taxlocation = "";
    m_customerText = "";

    m_isR2B = false;
    m_isReport = false;
    m_totallyup = false;

    connect(this, &ReceiptItemModel::dataChanged, this, &ReceiptItemModel::itemChangedSlot);
}

ReceiptItemModel::~ReceiptItemModel()
{
}

void ReceiptItemModel::clear()
{
    QStandardItemModel::clear();
    m_currency = Database::getCurrency();
    m_taxlocation = Database::getTaxLocation();
    m_customerText = "";

    m_isR2B = false;
    m_isReport = false;
    m_totallyup = false;
    initPlugins();
}

void ReceiptItemModel::setReceiptTime(QDateTime receiptTime)
{
    m_receiptTime = receiptTime;
}

void ReceiptItemModel::setCustomerText(QString customerText)
{
    m_customerText = customerText;
}

void ReceiptItemModel::setCurrentReceiptNum(int id)
{
    m_currentReceipt = id;
}

int ReceiptItemModel::getReceiptNum()
{
    return m_currentReceipt;
}

bool ReceiptItemModel::finishReceipts(int payedBy, int id, bool isReport)
{

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    bool ok = false;
    ok = query.exec(QString("UPDATE globals SET value=%1 WHERE name='lastReceiptNum'").arg(m_currentReceipt));

    if (!ok) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    QBCMath sum = 0.0;
    QBCMath net = 0.0;

    if (!isReport) {

        QSqlQuery orders(dbc);
        orders.prepare(QString("SELECT orders.count, orders.gross, orders.tax, orders.discount FROM orders WHERE orders.receiptId=%1")
                       .arg(m_currentReceipt));

        ok = orders.exec();
        if (!ok) {
            qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
        }

        QrkSettings settings;

        while ( orders.next() )
        {
            QBCMath count = orders.value("count").toDouble();
            count.round(settings.value("decimalDigits", 2).toInt());
            QBCMath singlePrice = orders.value("gross").toDouble();
            singlePrice.round(2);
            double tax = QString::number(orders.value("tax").toDouble(),'f',2).toDouble();
            QBCMath discount = orders.value("discount").toDouble();
            discount.round(2);

            QBCMath gross = singlePrice * count;
            gross = gross - ((gross / 100) * discount.toDouble());
            gross.round(2);
            sum += gross;
            net += gross / (1.0 + tax / 100.0);
            qApp->processEvents();
        }
    }

    setReceiptTime(QDateTime::currentDateTime());
    query.prepare(QString("UPDATE receipts SET timestamp=:timestamp, infodate=:infodate, receiptNum=:receiptNum, payedBy=:payedBy, gross=:gross, net=:net, userId=:userId WHERE id=:receiptNum"));
    query.bindValue(":timestamp", m_receiptTime.toString(Qt::ISODate));
    query.bindValue(":infodate", m_receiptTime.toString(Qt::ISODate));
    query.bindValue(":receiptNum", m_currentReceipt);
    query.bindValue(":payedBy", payedBy);
    query.bindValue(":gross", sum.toDouble());
    query.bindValue(":net", net.toString());
    query.bindValue(":userId", RBAC::Instance()->getUserId());

    ok = query.exec();
    if (!ok) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    QJsonObject data = compileData(id);
    if (!m_isReport && m_isR2B){
        data["isR2B"] = m_isR2B;
    }

    if (payedBy == PAYED_BY_CASH && m_given.value(PAYED_BY_CASH, 0.0) > 0.0) {
        QSqlDatabase dbc = Database::database();
        QSqlQuery query(dbc) ;
        query.prepare(QString("INSERT INTO receiptspay (receiptNum, payedBy, gross) VALUES (:receiptNum, :payedBy, :gross)"));
        query.bindValue(":receiptNum", m_currentReceipt);
        query.bindValue(":payedBy", PAYED_BY_CASH);
        query.bindValue(":gross", m_given.value(PAYED_BY_CASH));
        query.exec();
        if (m_given.value(PAYED_BY_DEBITCARD, 0.0) > 0.0) {
            query.bindValue(":payedBy", PAYED_BY_DEBITCARD);
            query.bindValue(":gross", m_given.value(PAYED_BY_DEBITCARD));
            query.exec();
        }
        if (m_given.value(PAYED_BY_CREDITCARD, 0.0) > 0.0) {
            query.bindValue(":payedBy", PAYED_BY_CREDITCARD);
            query.bindValue(":gross", m_given.value(PAYED_BY_CREDITCARD));
            query.exec();
        }
    }

    if (RKSignatureModule::isDEPactive()) {
        Utils utils;
        QString signature = utils.getSignature(data);
        if (signature.isEmpty()) {
            qCritical() << "Function Name: " << Q_FUNC_INFO << " No Signature Data: " << signature;
            return false;
        }

        query.prepare(QString("INSERT INTO dep (receiptNum, data) VALUES (:receiptNum, :data)"));
        query.bindValue(":receiptNum", m_currentReceipt);
        query.bindValue(":data", signature);

        qDebug() << "Function Name: " << Q_FUNC_INFO << " Signature Data: " << signature;

        ok = query.exec();
        if (!ok) {
            qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
        }
    }
    if (isReport)
        return true;

    if (id)
        Database::setStornoId(m_currentReceipt, id);

    if (ok) {
        DocumentPrinter p;
        p.printReceipt(data);

        Journal journal;
        journal.journalInsertReceipt(data);
    }

    return ok;

}

QJsonObject ReceiptItemModel::compileData(int id)
{

    QSqlDatabase dbc = Database::database();

    QSqlQuery query(dbc);
    QJsonObject Root;//root object

    // receiptNum, ReceiptTime
    query.prepare(QString("SELECT `receiptNum`,`timestamp`, `payedBy` FROM receipts WHERE id=%1").arg(m_currentReceipt));
    bool ok = query.exec();

    if (!ok) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();
    int receiptNum = query.value(0).toInt();
    QDateTime receiptTime = query.value(1).toDateTime();
    int payedBy = query.value(2).toInt();

    if (!m_customerText.isEmpty())
        Database::addCustomerText(receiptNum, m_customerText);

    // Positions
    query.prepare(QString("SELECT COUNT(*) FROM orders WHERE receiptId=%1").arg(m_currentReceipt));
    ok = query.exec();

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();
    int positions = query.value(0).toInt();

    // sum Year
    int year = receiptTime.toString("yyyy").toInt();
/*
    query.prepare(QString("SELECT SUM(gross) AS Total FROM receipts where timestamp like '%1%'").arg(year));
    query.exec();
    query.next();
    double sumYear = query.value(0).toDouble();
*/
    double sumYear = Utils::getYearlyTotal(year);

    // AdvertisingText
    Root["printAdvertisingText"] = Database::getAdvertisingText();

    // Header
    Root["printHeader"] = Database::getHeaderText();

    // Footer
    Root["printFooter"] = Database::getFooterText();

    // TaxTypes

    QSqlQuery taxTypes(dbc);
    taxTypes.prepare(QString("SELECT tax, comment FROM taxTypes WHERE taxlocation=:taxlocation ORDER BY id"));
    query.bindValue(":taxlocation", m_taxlocation);
    taxTypes.exec();
    while(taxTypes.next())
    {
        Root[taxTypes.value(1).toString()] = 0.0;
    }

    // Orders
    QSqlQuery orders(dbc);
    orders.prepare(QString("SELECT orders.count, products.name, orders.gross, orders.tax, products.coupon, orders.discount, products.itemnum FROM orders INNER JOIN products ON products.id=orders.product WHERE orders.receiptId=:id"));
    orders.bindValue(":id", m_currentReceipt);

    orders.exec();

    QrkSettings settings;

    // ZNr Programmversion Kassen-Id Beleg Belegtyp Bemerkung Nachbonierung
    // Belegnummer Datum Umsatz_Normal Umsatz_Ermaessigt1 Umsatz_Ermaessigt2
    // Umsatz_Null Umsatz_Besonders Jahresumsatz_bisher Erstellungsdatum

    Root["version"] = QString("%1.%2").arg(QRK_VERSION_MAJOR).arg(QRK_VERSION_MINOR);
    Root["action"] = JOURNAL_RECEIPT;
    Root["kasse"] = Database::getCashRegisterId();
    Root["displayname"] = RBAC::Instance()->getDisplayname();
    Root["actionText"] = tr("Beleg");
    Root["typeText"] = Database::getActionType((payedBy > PAYED_BY_START_RECEIPT)?0:payedBy);
    Root["shopName"] = Database::getShopName();
    Root["shopMasterData"] = Database::getShopMasterData();
    Root["headerText"] = m_customerText;
    Root["totallyup"] = (m_totallyup)? "Nachbonierung":"";
    Root["comment"] = (id > 0)? tr("Storno für Beleg Nr: %1").arg(id):settings.value("receiptPrinterHeading", "KASSABON").toString();
    Root["isStorno"] = (id > 0);
    Root["receiptNum"] = receiptNum;
    Root["receiptTime"] = receiptTime.toString(Qt::ISODate);
    Root["currentRegisterYear"] = QDate::currentDate().year();

    QJsonArray Orders;

    QBCMath sum(0.00);
    QMap<double, double> taxes; // <tax-percent, sum>

    while(orders.next()) //load all data from the database
    {
        QBCMath discount = orders.value("discount").toDouble();
        discount.round(2);
        QBCMath count(orders.value(0).toDouble());
        count.round(settings.value("decimalDigits", 2).toInt());
        QBCMath singlePrice(orders.value(2).toDouble());
        QBCMath gross = singlePrice * count;
        gross = gross - ((gross / 100) * discount);
        gross.round(2);
//        gross = QString::number(gross, 'f', 2).toDouble();
        double tax = orders.value(3).toDouble();

        sum += gross;

        if ( taxes.contains(tax) )
            taxes[tax] += Utils::getTax(gross.toDouble(), tax);
        else
            taxes[tax] = Utils::getTax(gross.toDouble(), tax);

        QJsonObject order;
        order["count"] = count.toDouble();
        order["itemNum"] = orders.value("itemNum").toString();
        order["product"] = orders.value(1).toString();
        order["discount"] = discount.toDouble();
        order["gross"] = gross.toDouble();
        order["singleprice"] = singlePrice.toDouble();
        order["tax"] = tax;
        order["coupon"] = orders.value(4).toString();
        Orders.append(order);

        QString taxType = Database::getTaxType(tax);
        Root[taxType] = Root[taxType].toDouble() + gross.toDouble(); /* last Info: we need GROSS :)*/
        qApp->processEvents();
    }

    QJsonArray Taxes;
    QList<double> keys = taxes.keys();
    for (int i = 0; i < keys.count(); i++)
    {
        QJsonObject tax;
        tax["t1"] = QString("%1%").arg( keys[i] );
        tax["t2"] = QString::number(taxes[keys[i]], 'f', 2);
        Taxes.append(tax);
    }

    Root["Orders"] = Orders;
    Root["sum"] = sum.toDouble();
    Root["sumYear"] = sumYear;
    Root["positions"] = positions;
    Root["Taxes"] = Taxes;
    Root["taxesCount"] = taxes.count();

    return Root;

}

void ReceiptItemModel::newOrder( bool  addRow )
{

    m_currentReceipt = 0;  // a new receipt not yet in the DB

    clear();

    setColumnCount(9);
    setHeaderData(REGISTER_COL_COUNT, Qt::Horizontal, QObject::tr("Anzahl"));
    setHeaderData(REGISTER_COL_PRODUCTNUMBER, Qt::Horizontal, QObject::tr("Artikelnummer"));
    setHeaderData(REGISTER_COL_PRODUCT, Qt::Horizontal, QObject::tr("Artikel"));
    setHeaderData(REGISTER_COL_NET, Qt::Horizontal, QObject::tr("E-Netto"));
    setHeaderData(REGISTER_COL_TAX, Qt::Horizontal, QObject::tr("MwSt."));
    setHeaderData(REGISTER_COL_SINGLE, Qt::Horizontal, QObject::tr("E-Preis"));
    setHeaderData(REGISTER_COL_DISCOUNT, Qt::Horizontal, QObject::tr("Rabatt %"));
    setHeaderData(REGISTER_COL_TOTAL, Qt::Horizontal, QObject::tr("Preis"));
    setHeaderData(REGISTER_COL_SAVE, Qt::Horizontal, QObject::tr(" "));

    if (addRow)
        plus();
}

void ReceiptItemModel::plus()
{
    int row = rowCount();
    m_manualProductsNumber = "";

    insertRow(row);
    blockSignals(true);

    QString defaultTax = Database::getDefaultTax();

    setColumnCount(9);
    setItem(row, REGISTER_COL_COUNT, new QStandardItem(QString("1")));
    setItem(row, REGISTER_COL_PRODUCTNUMBER, new QStandardItem(QString("")));
    setItem(row, REGISTER_COL_PRODUCT, new QStandardItem(QString("")));
    setItem(row, REGISTER_COL_TAX, new QStandardItem(defaultTax));
    setItem(row, REGISTER_COL_NET, new QStandardItem(QString("0")));
    setItem(row, REGISTER_COL_SINGLE, new QStandardItem(QString("0")));
    setItem(row, REGISTER_COL_DISCOUNT, new QStandardItem("0"));
    setItem(row, REGISTER_COL_TOTAL, new QStandardItem(QString("0")));

    QStandardItem* itemSave = new QStandardItem(false);
    itemSave->setCheckable(true);
    itemSave->setCheckState(Qt::Unchecked);
    itemSave->setEditable(false);
    itemSave->setText(tr("Speichern"));

    setItem(row, REGISTER_COL_SAVE, itemSave);

    item(row ,REGISTER_COL_COUNT)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    item(row ,REGISTER_COL_TAX)->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);

    blockSignals(false);
    emit finishedPlus();

}

bool ReceiptItemModel::createNullReceipt(int type)
{

    QString typeText = "";
    int payType = 0;
    bool mustSign = false;
    switch (type) {
    case START_RECEIPT:
        typeText = "Startbeleg";
        payType = PAYED_BY_START_RECEIPT;
        mustSign = true;
        break;
    case MONTH_RECEIPT:
        typeText = "Monatsbeleg";
        payType = PAYED_BY_MONTH_RECEIPT;
        break;
    case YEAR_RECEIPT:
        typeText = "Jahresbeleg";
        payType = PAYED_BY_MONTH_RECEIPT;
        mustSign = true;
        break;
    case COLLECTING_RECEIPT:
        typeText = QString("Sammelbeleg - Signatureinheit ausgefallen von %1 bis %2").arg(RKSignatureModule::resetSignatureModuleDamaged()).arg(QDateTime::currentDateTime().toString(Qt::ISODate));
        payType = PAYED_BY_COLLECTING_RECEIPT;
        mustSign = true;
        break;
    case CONTROL_RECEIPT:
        typeText = "Kontroll NULL Beleg";
        payType = PAYED_BY_CONTROL_RECEIPT;
        break;
    case CONCLUSION_RECEIPT:
        typeText = "SchlussBeleg";
        payType = PAYED_BY_CONCLUSION_RECEIPT;
        break;

    default:
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: False type";
        return false;
    }

    bool ret = false;
    newOrder();
    int rc = rowCount();
    if (rc == 0) {
        plus();
        rc = rowCount();
    }

    removeColumn(REGISTER_COL_SAVE);

    QJsonObject itemdata;
    itemdata["name"] = typeText;
    itemdata["tax"] = 0.0;
    itemdata["net"] = 0.0;
    itemdata["gross"] = 0.0;
    itemdata["visible"] = 0;
    itemdata["group"] = 1;


    ret = Database::addProduct(itemdata);
    if (ret) {
        item(rc -1, REGISTER_COL_COUNT)->setText( "1" );
        item(rc -1, REGISTER_COL_PRODUCT)->setText( typeText );
        item(rc -1, REGISTER_COL_TAX)->setText( "0" );
        item(rc -1, REGISTER_COL_SINGLE)->setText( "0" );

    } else {
        return false;
    }

    if (mustSign) {
        RKSignatureModule *sig = RKSignatureModuleFactory::createInstance("", DemoMode::isDemoMode());
        QString serial = sig->getCertificateSerial(true);
        delete sig;
        if (serial.isEmpty() || serial == "0") {
            qCritical() << "Function Name: " << Q_FUNC_INFO << " A receipt requiring a signature could not be created. Signature unit failed.";
            return false;
        }
    }

    if (int id = createReceipts()) {
        setCurrentReceiptNum(id);
        if (createOrder()) {
            if (type == START_RECEIPT)
                RKSignatureModule::setDEPactive(true);
            if (finishReceipts( payType )) {
                return true;
            }
        }
    }

    return false;
}

bool ReceiptItemModel::createStartReceipt()
{
    if (RKSignatureModule::isDEPactive())
        return false;

    bool ret = createNullReceipt(START_RECEIPT);

    if (ret)
        return true;

    RKSignatureModule::setDEPactive(false);
    return false;
}

int ReceiptItemModel::createReceipts()
{

    // Check if RKSignatureModule
    if (RKSignatureModule::isDEPactive() && RKSignatureModule::isSignatureModuleSetDamaged()) {
        RKSignatureModule *sigModule = RKSignatureModuleFactory::createInstance("", DemoMode::isDemoMode());
        sigModule->selectApplication();
        int certificateSerial = sigModule->getCertificateSerial(false).toInt();
        delete sigModule;
        if (certificateSerial != 0) {
            ReceiptItemModel receipt;
            receipt.createNullReceipt(COLLECTING_RECEIPT);
        }
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    bool processed = false;
    if (wsdlInterface) {
        processed = wsdlInterface->process(m_currentReceipt);
        if (!processed)
            qWarning() << "Function Name: " << Q_FUNC_INFO << " WSDL: " << processed;
    }

    int ok = query.exec(QString("INSERT INTO receipts (timestamp, infodate) VALUES('%1','%2')")
                        .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                        .arg(QDateTime::currentDateTime().toString(Qt::ISODate)
                        ));
    if (!ok) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
        return false;
    }

    QString driverName = dbc.driverName();
    if ( driverName == "QMYSQL" ) {
        query.prepare("SELECT LAST_INSERT_ID()");
        query.exec();
    }
    else if ( driverName == "QSQLITE" ) {
        query.prepare("SELECT last_insert_rowid()");
        query.exec();
    }

    query.next();
    m_currentReceipt = query.value(0).toInt();

    return m_currentReceipt;
}

bool ReceiptItemModel::createOrder(bool storno)
{

    bool ret = false;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc) ;
    query.prepare(QString("INSERT INTO orders (receiptId, product, count, net, discount, gross, tax) SELECT :receiptId, id, :count, :net, :discount, :egross, :tax FROM products WHERE name=:name LIMIT 1"));

    QrkSettings settings;

    int row_count = rowCount();
    for (int row = 0; row < row_count; row++)
    {
        QBCMath count(data(index(row, REGISTER_COL_COUNT, QModelIndex())).toDouble());
        count.round(settings.value("decimalDigits", 2).toInt());
        if (storno)
            count *= -1;

        QString product = data(index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString();

        QBCMath tax(data(index(row, REGISTER_COL_TAX, QModelIndex())).toDouble());
        QBCMath egross(data(index(row, REGISTER_COL_SINGLE, QModelIndex())).toDouble());
        QBCMath discount(data(index(row, REGISTER_COL_DISCOUNT, QModelIndex())).toDouble());
        tax.round(2);
        egross.round(2);
        discount.round(2);

        Database::updateProductSold(count.toDouble(), product);

        QBCMath net(egross - Utils::getTax(egross.toDouble(), tax.toDouble()));
        net.round(2);

        query.bindValue(":receiptId", m_currentReceipt);
        query.bindValue(":count", count.toDouble());
        query.bindValue(":net", net.toDouble());
        query.bindValue(":discount", discount.toDouble());
        query.bindValue(":egross", egross.toDouble());
        query.bindValue(":tax", tax.toDouble());
        query.bindValue(":name", product);

        ret = query.exec();

        if (!ret) {
            qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qCritical() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
        }
    }

    query.clear();

    return ret;
}

bool ReceiptItemModel::setR2BServerMode(QJsonObject obj)
{
    QString product = QString("Zahlungsbeleg für Rechnung %1 - nicht für den Vorsteuerabzug geeignet" ).arg(obj.value("receiptNum").toString());

    QString gross = Utils::normalizeNumber( obj.value("gross").toString() );

    if (!Utils::isNumber(gross)) { emit not_a_number("gross"); return false; }

    item(0, REGISTER_COL_COUNT)->setText( "1" );
    item(0, REGISTER_COL_PRODUCT)->setText( product );
    item(0, REGISTER_COL_TAX)->setText( "0" );
    item(0, REGISTER_COL_SINGLE)->setText( gross );

    if (!obj.value("customerText").toString().isEmpty())
        ReceiptItemModel::setCustomerText(obj.value("customerText").toString());

    if (!obj.value("given").isUndefined() && Utils::isNumber(obj.value("given").toString().toDouble()))
        ReceiptItemModel::setGiven(PAYED_BY_CASH, obj.value("given").toString().toDouble());

    m_isR2B = true;

    QJsonObject itemdata;
    itemdata["name"] = data(index(0, REGISTER_COL_PRODUCT, QModelIndex())).toString();
    itemdata["itemnum"] = data(index(0, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toString();
    itemdata["tax"] = data(index(0, REGISTER_COL_TAX, QModelIndex())).toDouble();
    itemdata["net"] = data(index(0, REGISTER_COL_NET, QModelIndex())).toDouble();
    itemdata["gross"] = data(index(0, REGISTER_COL_SINGLE, QModelIndex())).toDouble();
    itemdata["visible"] = 1;

    bool ret = Database::addProduct(itemdata);

    return ret;
}

bool ReceiptItemModel::setReceiptServerMode(QJsonObject obj)
{

    int rc = rowCount();
    QJsonArray receiptItems = obj.value("items").toArray();
    bool ok = false;
    bool ret = false;
    foreach (const QJsonValue & value, receiptItems) {
        QJsonObject jsonItem = value.toObject();
        ok = jsonItem.contains("count") && jsonItem.contains("name") && jsonItem.contains("gross") && jsonItem.contains("tax");
        if(!ok)
            continue;

        QString gross = Utils::normalizeNumber(jsonItem.value("gross").toString());
        QString net = Utils::normalizeNumber(jsonItem.value("net").toString());
        QString tax = Utils::normalizeNumber(jsonItem.value("tax").toString());
        QString count = Utils::normalizeNumber(jsonItem.value("count").toString());
        QString discount;
        if (jsonItem.value("discount").isUndefined()) {
            discount = "0.00";
        } else {
            discount = jsonItem.value("discount").toString();
            discount.replace("-","");
        }

        if (!Utils::isNumber(gross)) { emit not_a_number("gross"); return false; }
        if (!Utils::isNumber(tax)) { emit not_a_number("tax"); return false; }
        if (!Utils::isNumber(count)) { emit not_a_number("count"); return false; }
        if (!Utils::isNumber(discount)) { emit not_a_number("discount"); return false; }

        QJsonObject itemdata;
        itemdata["name"] = jsonItem.value("name").toString();
        itemdata["tax"] = tax.toDouble();
        itemdata["net"] = net.toDouble();
        itemdata["gross"] = gross.toDouble();
        itemdata["visible"] = 1;

        ret = Database::addProduct(itemdata);

        if (ret) {

            bool newItem = item(rc -1, REGISTER_COL_PRODUCT)->text().isEmpty();
            if (!newItem) {
                plus();
                rc = rowCount();
            }

            item(rc -1, REGISTER_COL_COUNT)->setText( count );
            item(rc -1, REGISTER_COL_PRODUCT)->setText( jsonItem.value("name").toString() );
            item(rc -1, REGISTER_COL_TAX)->setText( tax );
            item(rc -1, REGISTER_COL_DISCOUNT)->setText( discount );
            item(rc -1, REGISTER_COL_SINGLE)->setText( gross );

        } else {
            ret = false;
            break;
        }
    }

    QString customerText = obj.value("customerText").toString();
    if (! customerText.isEmpty())
        ReceiptItemModel::setCustomerText(obj.value("customerText").toString());

    if (!obj.value("given").isUndefined() && Utils::isNumber(obj.value("given").toString().toDouble()))
        ReceiptItemModel::setGiven(PAYED_BY_CASH,obj.value("given").toString().toDouble());

    return ret;
}

int ReceiptItemModel::getFreeProductNumber(int number, int currentRow)
{

    QString itemData = data(index(currentRow, REGISTER_COL_PRODUCT, QModelIndex())).toString();
    int row_count = rowCount();
    for (int row = 0; row < row_count; row++)
    {
        if (row == currentRow)
            continue;
        if (number == data(index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toInt()) {
            if (data(index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString() != itemData) {
                number = getFreeProductNumber(number+1, row);
            }
        } else {
            if (data(index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString() == itemData) {
                qDebug() << "Function Name: " << Q_FUNC_INFO << " Data: " << m_manualProductsNumber;
                blockSignals(true);
                item(currentRow, REGISTER_COL_PRODUCTNUMBER)->setText(data(index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toString());
                item(currentRow, REGISTER_COL_PRODUCT)->setText(data(index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString());
                blockSignals(false);
                item(currentRow, REGISTER_COL_TAX)->setText(data(index(row, REGISTER_COL_TAX, QModelIndex())).toString());
                item(currentRow, REGISTER_COL_SINGLE)->setText(data(index(row, REGISTER_COL_SINGLE, QModelIndex())).toString());
                qDebug() << "Function Name: " << Q_FUNC_INFO << " Data Single: " << data(index(row, REGISTER_COL_SINGLE, QModelIndex())).toString();
                return data(index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toInt();
            }
        }
    }
    return number;
}

QString ReceiptItemModel::getFreeProductNumber(QString number, int currentRow)
{

    QString itemData = data(index(currentRow, REGISTER_COL_PRODUCT, QModelIndex())).toString();
    int row_count = rowCount();
    for (int row = 0; row < row_count; row++)
    {
        if (row == currentRow)
            continue;
        if (number == data(index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toString()) {
            if (data(index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString() != itemData) {
                number = getFreeProductNumber(number.append("1"), row);
            }
        } else {
            if (data(index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString() == itemData) {
                qDebug() << "Function Name: " << Q_FUNC_INFO << " Data: " << m_manualProductsNumber;
                blockSignals(true);
                item(currentRow, REGISTER_COL_PRODUCTNUMBER)->setText(data(index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toString());
                item(currentRow, REGISTER_COL_PRODUCT)->setText(data(index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString());
                blockSignals(false);
                item(currentRow, REGISTER_COL_TAX)->setText(data(index(row, REGISTER_COL_TAX, QModelIndex())).toString());
                item(currentRow, REGISTER_COL_SINGLE)->setText(data(index(row, REGISTER_COL_SINGLE, QModelIndex())).toString());
                qDebug() << "Function Name: " << Q_FUNC_INFO << " Data Single: " << data(index(row, REGISTER_COL_SINGLE, QModelIndex())).toString();
                return data(index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toString();
            }
        }
    }
    return number;
}

void ReceiptItemModel::itemChangedSlot( const QModelIndex& i, const QModelIndex&)
{

    int row = i.row();
    int col = i.column();

    QBCMath sum(0.0);
    QBCMath net(0.0);
    QBCMath tax(0.0);

    QString s = data(index(row, col, QModelIndex())).toString();

    //  QString s2;
    QStringList temp = data(index(row, REGISTER_COL_SINGLE, QModelIndex())).toString().split(" ");
    double d2 = temp[0].replace(",", ".").toDouble();

    /* initialize QSqlDatabase dbc & QSqlQuery query(dbc)
   * will not work in a switch block
   */

    if (col == REGISTER_COL_PRODUCT) {
        QSqlDatabase dbc = Database::database();
        QSqlQuery query(dbc);
        query.prepare(QString("SELECT itemnum, gross, tax, lastchange FROM products WHERE name=:name"));
        query.bindValue(":name", s);
        query.exec();

        if (m_manualProductsNumber.isEmpty()) {
//            m_manualProductsNumber = data(index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toString();
            m_manualProductsNumber = Database::getNextProductNumber();
            qDebug() << "Function Name: " << Q_FUNC_INFO << " manual: " << m_manualProductsNumber;
        }

        m_changeProduct = true;

        if (!m_changeProductNumber && query.next()) {
            qDebug() << "Function Name: " << Q_FUNC_INFO << " ItemNum: " << query.value("itemnum").toString();
            blockSignals(true);
            setData(index(i.row(),0),tr("Letzte Datensatzänderung: %1").arg(QLocale().toString(query.value("lastchange").toDateTime())),Qt::ToolTipRole);
            item(row, REGISTER_COL_PRODUCTNUMBER)->setText(query.value("itemnum").toString());
            blockSignals(false);
            item(row, REGISTER_COL_TAX)->setText(query.value("tax").toString());
            item(row, REGISTER_COL_SINGLE)->setText(query.value("gross").toString());
        } else {
            qDebug() << "Function Name: " << Q_FUNC_INFO << " productNumber: " << data(index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toString();
            qDebug() << "Function Name: " << Q_FUNC_INFO << " manualProductNumber: " << m_manualProductsNumber;
            item(row, REGISTER_COL_SINGLE)->setText("0");
            if (!m_manualProductsNumber.isEmpty() && Utils::isNumber(m_manualProductsNumber)) {
                if (!Database::exists("products", m_manualProductsNumber.toInt(), "itemnum")) m_manualProductsNumber = QString::number(getFreeProductNumber(m_manualProductsNumber.toInt(), row));
                if (Database::exists("products", m_manualProductsNumber.toInt(), "itemnum")) m_manualProductsNumber = Database::getNextProductNumber();
            } else if (!m_manualProductsNumber.isEmpty()) {
                if (!Database::exists("products", m_manualProductsNumber, "itemnum")) m_manualProductsNumber = getFreeProductNumber(m_manualProductsNumber, row);
                if (Database::exists("products", m_manualProductsNumber, "itemnum")) m_manualProductsNumber = Database::getNextProductNumber();
            }
            blockSignals(true);
            item(row, REGISTER_COL_PRODUCTNUMBER)->setText(m_manualProductsNumber);
            item(row, REGISTER_COL_TAX)->setText(Database::getDefaultTax());
            blockSignals(false);
        }
        m_changeProduct = false;
        emit setButtonGroupEnabled(! s.isEmpty());
    }
    else if (col == REGISTER_COL_PRODUCTNUMBER && !s.isEmpty() ) {
        m_changeProductNumber = true;
        QSqlDatabase dbc = Database::database();
        QSqlQuery query(dbc);
        query.prepare(QString("SELECT name, gross, tax, lastchange FROM products WHERE itemnum=:itemnum"));
        query.bindValue(":itemnum", s);
        query.exec();
        if (!m_changeProduct && query.next()) {
            blockSignals(true);
            setData(index(i.row(),0),tr("Letzte Datensatzänderung: %1").arg(QLocale().toString(query.value("lastchange").toDateTime())),Qt::ToolTipRole);
            item(row, REGISTER_COL_PRODUCT)->setText(query.value("name").toString());
            blockSignals(false);
            item(row, REGISTER_COL_TAX)->setText(query.value("tax").toString());
            item(row, REGISTER_COL_SINGLE)->setText(query.value("gross").toString());
        } else {
            m_manualProductsNumber = s;
            item(row, REGISTER_COL_PRODUCT)->setText("");
            item(row, REGISTER_COL_SINGLE)->setText("0");
        }
        m_changeProductNumber = false;
    }
    else if (col == REGISTER_COL_PRODUCTNUMBER && s.isEmpty() ) {
        m_manualProductsNumber = s;
    }

    switch( col )
    {
    case REGISTER_COL_COUNT:
        if (s.toDouble() == 0.0) {
            sum = 0;
        } else {
            sum =  s.toDouble() * d2;
            sum = sum - ((sum / 100) * data(index(row, REGISTER_COL_DISCOUNT, QModelIndex())).toDouble());
        }

        sum.round(2);
        s = sum.toString();
//        if (s.toDouble() == 0)
            blockSignals(true);

        item(row, REGISTER_COL_TOTAL)->setText( s );

//        if (s.toDouble() == 0)
            blockSignals(false);

        break ;
    case REGISTER_COL_TAX:
        net = data(index(row, REGISTER_COL_NET, QModelIndex())).toDouble();
        sum = net * (1.0 + s.replace(" %", "").toDouble() / 100.0);
        sum.round(2);
        item(row, REGISTER_COL_SINGLE)->setText( sum.toString() );
        break;
    case REGISTER_COL_NET:
        s.replace(",", ".");
        blockSignals(true);
        tax = data(index(row, REGISTER_COL_TAX, QModelIndex())).toDouble();
        net = (s.toDouble() * ((100 + tax.toDouble()) / 100));
        net.round(2);
        s = net.toString();
        item(row, REGISTER_COL_SINGLE)->setText( s );
        sum = s.toDouble() * data(index(row, REGISTER_COL_COUNT, QModelIndex())).toDouble();
        sum = sum - ((sum / 100) * data(index(row, REGISTER_COL_DISCOUNT, QModelIndex())).toDouble());
        sum.round(2);
        s = sum.toString();
        item(row, REGISTER_COL_TOTAL)->setText( s );
        blockSignals(false);
        break;
    case REGISTER_COL_SINGLE:
        s.replace(",", ".");
        blockSignals(true);
        item(row, REGISTER_COL_SINGLE)->setText( s );
        tax = data(index(row, REGISTER_COL_TAX, QModelIndex())).toDouble();
        net = s.toDouble() / (1.0 + tax.toDouble() / 100.0);
        item(row, REGISTER_COL_NET)->setText( net.toString() );
        sum = s.toDouble() * data(index(row, REGISTER_COL_COUNT, QModelIndex())).toDouble();
        sum = sum - ((sum / 100) * data(index(row, REGISTER_COL_DISCOUNT, QModelIndex())).toDouble());
        sum.round(2);
        item(row, REGISTER_COL_TOTAL)->setText( sum.toString() );
        blockSignals(false);
        if (!m_changeProduct && !m_changeProductNumber)
            emit singlePriceChanged(data(index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString(), s, data(index(row, REGISTER_COL_TAX, QModelIndex())).toString());
        break ;
    case REGISTER_COL_DISCOUNT:
        blockSignals(true);
        s.replace(",", ".");
        s.replace("-", "");
        sum = data(index(row, REGISTER_COL_SINGLE, QModelIndex())).toDouble();
        sum = sum * data(index(row, REGISTER_COL_COUNT, QModelIndex())).toDouble();
        sum = sum - ((sum / 100) * s.toDouble());
        sum.round(2);
        item(row, REGISTER_COL_TOTAL)->setText( sum.toString() );
        blockSignals(false);
        break ;
    case REGISTER_COL_TOTAL:
        if (data(index(row, REGISTER_COL_COUNT, QModelIndex())).toDouble() == 0.00)
            break;

        s.replace(",", ".");
        s = QString::number(s.toDouble(), 'f', 2);
        blockSignals(true);
        item(row, REGISTER_COL_TOTAL)->setText( s );
        sum = s.toDouble() / data(index(row, REGISTER_COL_COUNT, QModelIndex())).toDouble();
        sum = (sum / (100 - data(index(row, REGISTER_COL_DISCOUNT, QModelIndex())).toDouble())) * 100;
        tax = data(index(row, REGISTER_COL_TAX, QModelIndex())).toDouble();
        net = sum / (1.0 + tax.toDouble() / 100.0);
        net.round(2);
        sum.round(2);
        item(row, REGISTER_COL_NET)->setText( net.toString() );
        item(row, REGISTER_COL_SINGLE)->setText( sum.toString() );
        blockSignals(false);
        break;
    }

    emit finishedItemChanged();

}

bool ReceiptItemModel::storno(int id)
{

    int rc = rowCount();

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT orders.count, products.name, orders.tax, orders.net, orders.gross, orders.discount FROM orders INNER JOIN products ON products.id=orders.product WHERE orders.receiptId=:id");
    query.bindValue(":id", id);

    bool ret = query.exec();

    if (!ret) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    while (query.next()) {
        bool newItem = item(rc -1, REGISTER_COL_PRODUCT)->text().isEmpty();
        if (!newItem) {
            plus();
            rc = rowCount();
        }

        item(rc -1, REGISTER_COL_COUNT)->setText(query.value(0).toString());
        item(rc -1, REGISTER_COL_PRODUCT)->setText(query.value(1).toString());
        item(rc -1, REGISTER_COL_TAX)->setText(query.value(2).toString());
        item(rc -1, REGISTER_COL_DISCOUNT)->setText(query.value("discount").toString());
        item(rc -1, REGISTER_COL_NET)->setText(query.value(3).toString());
        item(rc -1, REGISTER_COL_SINGLE)->setText(query.value(4).toString());
    }

    return ret;
}

void ReceiptItemModel::initPlugins()
{
    if (!wsdlInterface)
        wsdlInterface = qobject_cast<WsdlInterface *>(PluginManager::instance()->getObjectByName("Wsdl*"));

    if (!wsdlInterface)
        qDebug() << "Function Name: " << Q_FUNC_INFO << " WSDL: not available";
}

void ReceiptItemModel::setGiven(int payed, double given)
{
    m_given.insert(payed, given);
}

void ReceiptItemModel::setGiven(QMap<int, double> given)
{
    m_given = given;
}
