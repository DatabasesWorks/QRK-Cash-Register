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

#ifndef DATABASE
#define DATABASE

#include <QObject>
#include "qrkcore_global.h"

class QSqlQuery;
class QSqlDatabase;

class QRK_EXPORT Database : public QObject
{
    Q_OBJECT
  public:
    Database(QObject *parent = Q_NULLPTR);
    ~Database();

    static QJsonObject getConnectionDefinition();
    static bool open(bool dbSelect, QString TESTFILE = "");
    static void reopen();
    static QDateTime getLastJournalEntryDate();
    static QString getLastExecutedQuery(const QSqlQuery &query);
    static QString getShopName();
    static QString getShopMasterData();
    static QStringList getLastReceipt();
    static QDate getFirstReceiptDate();
    static QDateTime getFirstReceiptDateTime();
    static QDate getLastReceiptDate();
    static QDateTime getLastReceiptDateTime();
    static bool addCustomerText(int id, QString text);
    static QString getCustomerText(int id);
    static int getProductIdByName(QString name);
    static int getProductIdByNumber(QString number);
    static int getProductIdByBarcode(QString code);
    static QString getProductNameById(int id);

    static bool addProduct(const QJsonObject &data);
    static QJsonObject getProductByName(QString name, int visible = 1);
    static QJsonObject getProductById(int id, int visible = 1);

    static bool exists(const QString &name);
    static bool exists(const QString type, const int &id, const QString fieldname = "id");
    static bool exists(const QString type, const QString &name, const QString fieldname)
;
    static int getPayedBy(int);
    static QMap<int, double> getGiven(int id);
    static int getActionTypeByName(const QString &name);
    static QString getActionType(int id);
    static QString getTaxType(double id);
    static void setStornoId(int, int);
    static int getStorno(int);
    static int getStornoId(int);
    static QString getCashRegisterId();
    static QString getCurrency();
    static QString getShortCurrency();
    static QString getTaxLocation();
    static QString getDefaultTax();
    static QString getAdvertisingText();
    static QString getHeaderText();
    static QString getFooterText();

    static int getLastReceiptNum(bool realReceipt = false);
    static QString getDayCounter();
    static QString getMonthCounter();
    static QString getYearCounter();
    static QString getSalesPerPaymentSQLQueryString();
    static void updateProductSold(double, QString);
    static void updateProductPrice(double, QString);
    static bool moveProductsToDefaultGroup(int oldgroupid);
    static void insertProductItemnumToExistingProduct(QString itemnum, QString product);
    static QString getNextProductNumber();


    static QStringList getStockInfoList();
    static QStringList getMaximumItemSold();
    static void setCashRegisterInAktive();
    static bool isCashRegisterInAktive();
    static void resetAllData();
    static QString getLastVersionInfo();
    static void cleanup();
    static QString updateGlobals(QString name, QString defaultvalue, QString defaultStrValue);
    static QSqlDatabase database(const QString &connectionname = "CN");
    static bool isAnyValueFunctionAvailable();
    static QString getDatabaseVersion();
    static QStringList getDatabaseTableHeaderNames(QString tablename);

  private:
    static QString getDatabaseType();
    static void setStorno(int,int = 1);

};

extern QMap<QString, QString> globalStringValues;

#endif // DATABASE
