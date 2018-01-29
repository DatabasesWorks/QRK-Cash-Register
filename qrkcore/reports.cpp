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
#include "utils/utils.h"
#include "reports.h"
#include "documentprinter.h"
#include "backup.h"
#include "export.h"
#include "qrkprogress.h"
#include "singleton/spreadsignal.h"
#include "RK/rk_signaturemodule.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "defines.h"

#include <QApplication>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QJsonObject>
#include <QTextDocument>
#include <QDebug>

Reports::Reports(QObject *parent, bool servermode)
    : ReceiptItemModel(parent), m_servermode(servermode)
{
}

//--------------------------------------------------------------------------------

Reports::~Reports()
{
    Spread::Instance()->setProgressBarValue(-1);
}

/**
 * @brief Reports::getEOFMap
 * @param checkDate
 * @return
 */
QMap<int, QDate> Reports::getEOFMap(QDate checkDate)
{
    QDate last = Database::getLastReceiptDate();
    QMap<int, QDate> map;
    QDate lastEOD = getLastEOD();
    int type = getReportType();

    if (type == PAYED_BY_REPORT_EOM || type == PAYED_BY_MONTH_RECEIPT){
        type = PAYED_BY_REPORT_EOM;
        last = lastEOD;
    }

    if (type == -1)
        return QMap<int, QDate>();

    // Tagesabschluss von Heute schon gemacht?
    if (lastEOD.isValid() && lastEOD == checkDate) {
        map.insert(PAYED_BY_REPORT_EOD, QDate());
        return map;
    }

    if (last.isValid() && !(type ==  PAYED_BY_REPORT_EOD) && !(type == PAYED_BY_REPORT_EOM)&& checkDate != last)
        map.insert(PAYED_BY_REPORT_EOD, last);

    QString lastMonth = last.toString("yyyyMM");
    QString checkMonth = checkDate.toString("yyyyMM");

    // Monatsabschluss von diesen Monat schon gemacht?
    if (type == PAYED_BY_REPORT_EOM && lastMonth == checkMonth) {
        map.insert(PAYED_BY_REPORT_EOM, QDate());
        return map;
    }
    if (lastEOD.isValid() && lastEOD > checkDate) {
        map.insert(PAYED_BY_REPORT_EOM, QDate());
        return map;
    }

    if (last.isValid() && !(lastMonth == checkMonth) && !(type ==  PAYED_BY_REPORT_EOM) && checkDate != last)
        map.insert(PAYED_BY_REPORT_EOM, last);

    lastMonth = last.addMonths(1).toString("yyyyMM");
    if ((lastMonth < checkMonth) && (type ==  PAYED_BY_REPORT_EOM) && checkDate != last)
        map.insert(PAYED_BY_REPORT_EOM, last.addMonths(1));

    return map;
}

/**
 * @brief Reports::checkEOAny
 * @param checkDate
 * @return
 */
bool Reports::checkEOAny(QDate checkDate, bool checkDay)
{
    bool ret = true;
    QMap<int, QDate> map = getEOFMap(checkDate);
    if (map.isEmpty())
        return true;

    if (map.contains(PAYED_BY_REPORT_EOD) && checkDay) {
        QDate date = map.value(PAYED_BY_REPORT_EOD);
        if (!date.isValid()) {
            if (!m_servermode)
                checkEOAnyMessageBoxInfo(PAYED_BY_REPORT_EOD, QDate::currentDate(), tr("Tagesabschluss wurde bereits erstellt."));
            return false;
        }
    }

    if (map.contains(PAYED_BY_REPORT_EOM)) {
        QDate date = map.value(PAYED_BY_REPORT_EOM);
        if (!date.isValid()) {
            if (!m_servermode)
                checkEOAnyMessageBoxInfo(PAYED_BY_REPORT_EOM, QDate::currentDate(), tr("Monatsabschluss %1 wurde bereits erstellt.").arg( QDate::longMonthName(QDate::currentDate().month())));
            return false;
        }
    }

    QMapIterator<int, QDate> i(map);
    while (ret && i.hasNext()) {
        i.next();

        if (!m_servermode)
            ret = checkEOAnyMessageBoxYesNo(i.key(), i.value());

        if(ret) {
            if (i.key() == PAYED_BY_REPORT_EOD && checkDay) {
                ret = endOfDay();
                if (m_servermode && ret)
                    Spread::Instance()->setImportInfo(tr("Tagesabschluss vom %1 wurde erstellt.").arg(i.value().toString()));
                else if (m_servermode)
                    Spread::Instance()->setImportInfo(tr("Tagesabschluss vom %1 konnte nicht erstellt werden.").arg(i.value().toString()));
            } else if (i.key() == PAYED_BY_REPORT_EOM) {
                ret = endOfMonth();
                if (m_servermode && ret)
                    Spread::Instance()->setImportInfo(tr("Monatsabschluss vom %1 wurde erstellt.").arg(i.value().toString()));
                else if (m_servermode) {
                    Spread::Instance()->setImportInfo(tr("Monatsabschluss vom %1 konnte nicht erstellt werden.").arg(i.value().toString()), true);
                    if (RKSignatureModule::isSignatureModuleSetDamaged())
                        Spread::Instance()->setImportInfo(tr("Ein Signaturpflichtiger Beleg konnte nicht erstellt werden. Signatureinheit ausgefallen."), true);
                }
                else if (!ret && RKSignatureModule::isSignatureModuleSetDamaged()) {
                    Spread::Instance()->setProgressBarValue(-1);
                    QString text = tr("Ein Signaturpflichtiger Beleg konnte nicht erstellt werden. Signatureinheit ausgefallen.");
                    checkEOAnyMessageBoxInfo(PAYED_BY_REPORT_EOM, QDate::currentDate(), text);
                }
            }
        }
    }

    return ret;
}

bool Reports::checkEOAnyServerMode()
{
    return checkEOAny();
}

/**
 * @brief Reports::checkEOAnyMessageBoxYesNo
 * @param type
 * @param date
 * @param text
 * @return
 */
bool Reports::checkEOAnyMessageBoxYesNo(int type, QDate date, QString text)
{
    QString infoText;
    if (type == PAYED_BY_REPORT_EOD) {
        infoText = tr("Tagesabschluss");
        if (text.isEmpty()) text = tr("Tagesabschluss vom %1 muß erstellt werden.").arg(date.toString());
    } else {
        infoText = tr("Monatsabschluss");
        if (text.isEmpty()) text = tr("Monatsabschluss für %1 muß erstellt werden.").arg(date.toString("MMMM yyyy"));
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle(infoText);

    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(text);
    msgBox.setStandardButtons(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Erstellen"));
    msgBox.setButtonText(QMessageBox::No, tr("Abbrechen"));
    msgBox.setDefaultButton(QMessageBox::No);

    if(msgBox.exec() == QMessageBox::Yes)
        return true;

    return false;
}

/**
 * @brief Reports::checkEOAnyMessageBoxInfo
 * @param type
 * @param date
 * @param text
 */
void Reports::checkEOAnyMessageBoxInfo(int type, QDate date, QString text)
{
    QString infoText;
    if (type == PAYED_BY_REPORT_EOD) {
        infoText = tr("Tagesabschluss");
    } else {
        infoText = tr("Monatsabschluss");
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle(infoText);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(text);
    msgBox.setInformativeText(tr("Erstellungsdatum %1").arg(date.toString()));
    msgBox.setStandardButtons(QMessageBox::Yes);
    msgBox.setButtonText(QMessageBox::Yes, tr("OK"));
    msgBox.exec();
}

bool Reports::endOfDay() {
    return endOfDay(true);
}

/**
 * @brief Reports::endOfDay
 * @return
 */
bool Reports::endOfDay(bool ask)
{
    QDate date = Database::getLastReceiptDate();

    bool create = canCreateEOD(date);
    if (create) {
        if (m_servermode)
            return doEndOfDay(date);

        QString text;
        bool ok = true;
        if (ask && date == QDate::currentDate()) {
            text = tr("Nach dem Erstellen des Tagesabschlusses ist eine Bonierung für den heutigen Tag nicht mehr möglich.");
            ok = checkEOAnyMessageBoxYesNo(PAYED_BY_REPORT_EOD, date, text);
        }

        if (ok) {
            QRKProgress progress;
            progress.setText(tr("Tagesabschluss wird erstellt."));
            progress.setWaitMode(true);
            progress.show();
            qApp->processEvents();

            return doEndOfDay(date);
        }
    } else {
        if (!m_servermode)
            checkEOAnyMessageBoxInfo(PAYED_BY_REPORT_EOD, date, tr("Tagesabschluss wurde bereits erstellt."));
    }

    return false;
}

/**
 * @brief Reports::doEndOfDay
 * @param date
 * @return
 */
bool Reports::doEndOfDay(QDate date)
{
    QSqlDatabase dbc = Database::database();

    Spread::Instance()->setProgressBarValue(1);
    Backup::create();
    dbc.transaction();
    m_currentReceipt = createReceipts();
    bool ret = finishReceipts(PAYED_BY_REPORT_EOD, 0, true);
    if (ret) {
        if (createEOD(m_currentReceipt, date)) {
            dbc.commit();
            printDocument(m_currentReceipt, tr("Tagesabschluss"));
        } else {
            dbc.rollback();
            return false;
        }
    } else {
        dbc.rollback();
        return false;
    }

    return ret;
}

/**
 * @brief Reports::endOfMonth
 * @return
 */
bool Reports::endOfMonth()
{
    QDate rDate = Database::getLastReceiptDate();
    int type = getReportType();

    if (type == PAYED_BY_REPORT_EOD) {
        rDate = getLastEOD();
    }

    if (type == PAYED_BY_REPORT_EOM || type == PAYED_BY_MONTH_RECEIPT) {
        rDate = getLastEOD().addMonths(1);
    }

    int receiptMonth = (rDate.year() * 100) + rDate.month();
    int currMonth = (QDate::currentDate().year() * 100) + QDate::currentDate().month();

    bool ok = (rDate.isValid() && receiptMonth <= currMonth);

    if (ok) {
        QDateTime checkdate = QDateTime::currentDateTime();

        if (QDate::currentDate().year() == rDate.year()) {
            checkdate.setDate(QDate::fromString(QString("%1-%2-1")
                                                .arg(rDate.year())
                                                .arg(rDate.month())
                                                , "yyyy-M-d")
                              .addMonths(1).addDays(-1));
        } else {
            checkdate.setDate(QDate::fromString(QString("%1-12-31")
                                                .arg(QDate::currentDate().year()),"yyyy-M-d")
                              .addYears(-1));
        }
        checkdate.setTime(QTime::fromString("23:59:59"));

        bool canCreateEom = canCreateEOM(rDate);

        if (!(type == PAYED_BY_REPORT_EOM) && !(type == PAYED_BY_MONTH_RECEIPT)) {
            bool canCreateEod = canCreateEOD(rDate);

            if (QDateTime(rDate) <= checkdate && canCreateEod) {
                bool doJob = true;
                if (!m_servermode)
                    doJob = checkEOAnyMessageBoxYesNo(PAYED_BY_REPORT_EOD, rDate,tr("Der Tagesabschlusses für %1 muß zuerst erstellt werden.").arg(rDate.toString()));

                if (doJob) {
                    if (! endOfDay())
                        return false;
                } else {
                    return false;
                }
            }
        }

        if (canCreateEom) {
            ok = true;
            if (m_servermode) {
                if (doEndOfMonth(checkdate.date())) {
                    rDate = rDate.addMonths(1);
                    receiptMonth = (rDate.year() * 100) + rDate.month();
                    if (rDate.isValid() && receiptMonth < currMonth)
                        ok = checkEOAny();
                } else {
                    ok = false;
                }
                return ok;
            }

            QRKProgress progress;
            progress.setText(tr("Monatsabschluss wird erstellt."));
            progress.setWaitMode(true);
            progress.show();
            qApp->processEvents();

            if (receiptMonth == currMonth) {
                QString text = tr("Nach dem Erstellen des Monatsabschlusses ist eine Bonierung für diesen Monat nicht mehr möglich.");
                ok = checkEOAnyMessageBoxYesNo(PAYED_BY_REPORT_EOM, rDate, text);
            }
            if (ok) {
                if (doEndOfMonth(checkdate.date())){
                    rDate = rDate.addMonths(1);
                    receiptMonth = (rDate.year() * 100) + rDate.month();
                    if (rDate.isValid() && receiptMonth < currMonth)
                        ok = checkEOAny();
                } else {
                    ok = false;
                }
            }
        }
    } else {
        QDate next = QDate::currentDate();
        next.setDate(next.year(), next.addMonths(1).month(), 1);
        QString text = tr("Der Monatsabschluss für %1 wurde schon gemacht. Der nächste Abschluss kann erst ab %2 gemacht werden.").arg(QDate::longMonthName(QDate::currentDate().month())).arg(next.toString());
        checkEOAnyMessageBoxInfo(PAYED_BY_REPORT_EOM, QDate::currentDate(), text);
    }

    return ok;
}

/**
 * @brief Reports::doEndOfMonth
 * @param date
 * @return
 */
bool Reports::doEndOfMonth(QDate date)
{
    QSqlDatabase dbc = Database::database();

    Spread::Instance()->setProgressBarValue(1);
    bool ret = false;
    Backup::create();
    clear();
    dbc.transaction();
    m_currentReceipt =  createReceipts();
    ret = finishReceipts(PAYED_BY_REPORT_EOM, 0, true);
    if (ret) {
        if (createEOM(m_currentReceipt, date)) {
            if (nullReceipt(date)) {
                dbc.commit();
                printDocument(m_currentReceipt, tr("Monatsabschluss"));
            } else {
                dbc.rollback();
                return false;
            }
        } else {
            dbc.rollback();
            return false;
        }
    } else {
        dbc.rollback();
        return false;
    }


    return ret;
}

bool Reports::nullReceipt(QDate date)
{
    bool ret = true;
    if (RKSignatureModule::isDEPactive()) {
        if (date.year() < QDate::currentDate().year() || date.month() == 12)
            ret = createNullReceipt(YEAR_RECEIPT);
        else
            ret = createNullReceipt(MONTH_RECEIPT);

        if (!ret)
            return false;

        int counter = -1;
        Export xport;
        bool exp = xport.createBackup(counter);
        if (!exp && counter < 1) {
            QString text = tr("Das automatische DEP-Backup konnte nicht durchgeführt werden.\nStellen Sie sicher das Ihr externes Medium zu Verfügung steht und sichern Sie das DEP manuell.");
            if (m_servermode)
                Spread::Instance()->setImportInfo(text, true);
            else
                checkEOAnyMessageBoxInfo(PAYED_BY_REPORT_EOM, QDate::currentDate(), text);
        }
    }

    return ret;
}

/**
 * @brief Reports::createEOD
 * @param id
 * @param date
 */
bool Reports::createEOD(int id, QDate date)
{
    QDateTime from;
    QDateTime to;

    // ---------- DAY -----------------------------
    from.setDate(QDate::fromString(date.toString()));
    to.setDate(QDate::fromString(date.toString()));
    to.setTime(QTime::fromString("23:59:59"));

    QStringList eod;
    eod.append(createStat(id, "Tagesumsatz", from, to));

    QString line = QString("Tagesbeleg\tTagesbeleg\t\t%1\t%2\t0,0\t0,0\t0,0\t0,0\t0,0\t%3")
            .arg(id)
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
            .arg(Utils::getYearlyTotal(date.year()));

    bool ret = insert(eod, id, to);

    Journal journal;
    journal.journalInsertLine("Beleg", line);

    Spread::Instance()->setProgressBarValue(100);

    return ret;
}

/**
 * @brief Reports::createEOM
 * @param id
 * @param date
 */
bool Reports::createEOM(int id, QDate date)
{

    QDateTime from;
    QDateTime to;

    // ---------- MONTH -----------------------------
    from.setDate(QDate::fromString(QString("%1-%2-01").arg(date.year()).arg(date.month()),"yyyy-M-d"));
    to.setDate(QDate::fromString(date.toString()));
    to.setTime(QTime::fromString("23:59:59"));

    QStringList eod;
    eod.append(createStat(id, "Monatsumsatz", from, to));

    // ----------- YEAR ---------------------------

    QString fromString = QString("%1-01-01").arg(date.year());
    from.setDate(QDate::fromString(fromString, "yyyy-MM-dd"));
    to.setDate(QDate::fromString(date.toString()));
    to.setTime(QTime::fromString("23:59:59"));

    if (date.month() == 12) {
        eod.append(createYearStat(id, date));
    }

    // ----------------------------------------------

    QString line = QString("Monatsbeleg\tMonatsbeleg\t\t%1\t%2\t0,0\t0,0\t0,0\t0,0\t0,0\t%3")
            .arg(id)
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
            .arg(Utils::getYearlyTotal( date.year() ));

    bool ret = insert(eod, id, to);

    Journal journal;
    journal.journalInsertLine("Beleg", line);

    Spread::Instance()->setProgressBarValue(100);

    return ret;
}

/**
 * @brief Reports::getLastEOD
 * @return
 */
QDate Reports::getLastEOD()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT max(timestamp) AS timestamp FROM reports");
    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    if (query.last()) {
        return query.value("timestamp").toDate();
    }

    return QDate();
}

/**
* @brief Reports::canCreateEOD
* @param date
* @return
*/
bool Reports::canCreateEOD(QDate date)
{
    QDateTime f;
    QDateTime t;
    f.setDate(QDate::fromString(date.toString()));
    t.setDate(QDate::fromString(date.toString()));
    f.setTime(QTime::fromString("00:00:00"));
    t.setTime(QTime::fromString("23:59:59"));

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT reports.timestamp FROM reports, receipts where reports.timestamp BETWEEN :fromDate AND :toDate AND receipts.payedBy > 2 AND reports.receiptNum=receipts.receiptNum ORDER BY receipts.timestamp DESC LIMIT 1");
    query.bindValue(":fromDate", f.toString(Qt::ISODate));
    query.bindValue(":toDate", t.toString(Qt::ISODate));

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    if (query.last()) {
        date = query.value("timestamp").toDate();
        return false;
    }

    return true;
}

/**
 * @brief Reports::canCreateEOM
 * @param date
 * @return
 */
bool Reports::canCreateEOM(QDate date)
{
    QDateTime f;
    QDateTime t;
    f.setDate(QDate::fromString(date.toString()));
    t.setDate(QDate::fromString(date.toString()));
    f.setTime(QTime::fromString("00:00:00"));
    t.setTime(QTime::fromString("23:59:59"));

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT reports.timestamp FROM reports, receipts where reports.timestamp BETWEEN :fromDate AND :toDate AND receipts.payedBy = 4 AND reports.receiptNum=receipts.receiptNum ORDER BY receipts.timestamp DESC LIMIT 1");
    query.bindValue(":fromDate", f.toString(Qt::ISODate));
    query.bindValue(":toDate", t.toString(Qt::ISODate));

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    if (query.last()) {
        date = query.value("timestamp").toDate();
        return false;
    }

    return true;
}

/**
 * @brief Reports::getReportType
 * @return
 */
int Reports::getReportType()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("select payedBy,receiptNum from receipts where id=(select max(id) from receipts);");
    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    if (query.last()) {
        if (query.value("payedBy").isNull())
            return -1;
        return query.value("payedBy").toInt();
    }
    return -1;
}

/**
 * @brief Reports::createStat
 * @param id
 * @param type
 * @param from
 * @param to
 * @return
 */
QStringList Reports::createStat(int id, QString type, QDateTime from, QDateTime to)
{

    if (to.toString("yyyyMMdd") == QDate::currentDate().toString("yyyyMMdd"))
        to.setTime(QTime::currentTime());

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    /* Anzahl verkaufter Artikel oder Leistungen */
//    query.prepare("SELECT sum(ROUND(orders.count,2)) as count FROM orders WHERE receiptId IN (SELECT id FROM receipts WHERE timestamp BETWEEN :fromDate AND :toDate AND payedBy <= 2)");
    query.prepare("SELECT sum(orders.count) as count FROM orders WHERE receiptId IN (SELECT id FROM receipts WHERE timestamp BETWEEN :fromDate AND :toDate AND payedBy <= 2)");
    query.bindValue(":fromDate", from.toString(Qt::ISODate));
    query.bindValue(":toDate", to.toString(Qt::ISODate));
    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();

    //double sumProducts = query.value("count").toDouble();
    QBCMath sumProducts(query.value("count").toDouble());
    sumProducts.round(2);

    QStringList stat;
    stat.append(QString("Anzahl verkaufter Artikel oder Leistungen: %1").arg(sumProducts.toString().replace(".",",")));

    /* Anzahl Zahlungen */
    query.prepare("SELECT count(id) as count_id FROM receipts WHERE timestamp BETWEEN :fromDate AND :toDate AND payedBy <= 2 AND storno < 2");
    query.bindValue(":fromDate", from.toString(Qt::ISODate));
    query.bindValue(":toDate", to.toString(Qt::ISODate));
    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();

    stat.append(QString("Anzahl Zahlungen: %1").arg(query.value("count_id").toInt()));

    /* Anzahl Stornos */
    query.prepare("SELECT count(id) as count_id FROM receipts WHERE timestamp BETWEEN :fromDate AND :toDate AND storno = 2");
    query.bindValue(":fromDate", from.toString(Qt::ISODate));
    query.bindValue(":toDate", to.toString(Qt::ISODate));

    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();

    stat.append(QString("Anzahl Stornos: %1").arg(query.value("count_id").toInt()));
    stat.append("-");

    /* Umsätze Zahlungsmittel */
    query.prepare(Database::getSalesPerPaymentSQLQueryString());
    query.bindValue(":fromDate", from.toString(Qt::ISODate));
    query.bindValue(":toDate", to.toString(Qt::ISODate));

    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    /*
     * FIXME: We do this workaroud, while SUM and ROUND will
     * give use the false result. SQL ROUND/SUM give xx.98 from xx.985
     * should be xx.99
     */

    stat.append(tr("Umsätze nach Zahlungsmittel"));
    QMap<QString, QMap<double, double> > zm;
    QMap<double, double> map;
    while (query.next())
    {
        QString key = query.value("actionText").toString();
        QBCMath tax(query.value("tax").toString());
        tax.round(2);
        QBCMath total(query.value("total").toString());
        total.round(2);
        if ( zm.contains(key) ) {
            map[tax.toDouble()] += total.toDouble();
            zm[key] = map;
        } else {
            map.clear();
            map[tax.toDouble()] = total.toDouble();
            zm[key] = map;
        }
    }

    QMap<QString, QMap<double, double> >::iterator i;
    for (i = zm.begin(); i != zm.end(); ++i) {
        QString key = i.key();
        stat.append(key);

        QMap<double, double> tax = i.value();
        QMap<double, double>::iterator j;
        QBCMath total(0.0);
        for (j = tax.begin(); j != tax.end(); ++j) {
            QBCMath k(j.key());
            QBCMath v(j.value());
            stat.append(QString("%1%: %2")
                         .arg(QBCMath::bcround(k.toString(), 2).replace(".",","))
                         .arg(QBCMath::bcround(v.toString(), 2).replace(".",",")));
            total += v.toString();
        }
        stat.append("-");
        stat.append(QString("Summe %1: %2").arg(key, QBCMath::bcround(total.toString(), 2).replace('.', ',')));
        stat.append("-");
    }

    /*
     * FIXME: We do this workaroud, while SUM and ROUND will
     * give use the false result. SQL ROUND/SUM give xx.98 from xx.985
     * should be xx.99
     */

    /* Umsätze Steuern */
    query.prepare("SELECT orders.tax, receipts.receiptNum, (orders.count * orders.gross) - (orders.count * orders.gross * orders.discount) / 100 as total from receipts LEFT JOIN orders on orders.receiptId=receipts.receiptNum WHERE receipts.timestamp between :fromDate AND :toDate AND receipts.payedBy < 3 ORDER BY orders.tax");
    query.bindValue(":fromDate", from.toString(Qt::ISODate));
    query.bindValue(":toDate", to.toString(Qt::ISODate));

    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    stat.append(tr("Umsätze nach Steuersätzen"));
    map.clear();
    while (query.next())
    {
        QBCMath key(query.value("tax").toString());
        key.round(2);
        QBCMath total(query.value("total").toString());
        total.round(2);
        if ( map.contains(key.toDouble()) ) {
            map[key.toDouble()] += total.toDouble();
        } else {
            map[key.toDouble()] = total.toDouble();
        }
    }

    QMap<double, double>::iterator j;
    for (j = map.begin(); j != map.end(); ++j) {
        QBCMath k(j.key());
        QBCMath v(j.value());
        stat.append(QString("%1%: %2")
                    .arg(QBCMath::bcround(k.toString(),2).replace(".",","))
                    .arg(QBCMath::bcround(v.toString(),2).replace(".",",")));
    }
    stat.append("-");

/*
    while (query.next())
    {
        if (tax.isEmpty()) tax = query.value("tax").toString();
        if (tax != query.value("tax").toString()) {
            stat.append(QString("%1%: %2")
                        .arg(tax)
                        .arg(QBCMath::bcround(total.toString(),2).replace(".",",")));
            tax = query.value("tax").toString();
            total = 0.0;
        }
        total += query.value("total").toDouble();
    }
    if (!tax.isEmpty()) {
        stat.append(QString("%1%: %2")
                    .arg(tax)
                    .arg(QBCMath::bcround(total.toString(), 2).replace(".",",")));
        total = 0.0;
    }

    stat.append("-");
*/
    /* Summe */
    query.prepare("SELECT sum(gross) as total FROM receipts WHERE timestamp BETWEEN :fromDate AND :toDate AND payedBy < 3");
    query.bindValue(":fromDate", from.toString(Qt::ISODate));
    query.bindValue(":toDate", to.toString(Qt::ISODate));
    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();

    QBCMath gross(query.value("total").toDouble());
    gross.round(2);
    QString sales = gross.toString().replace(".",",");

    if (type == "Jahresumsatz") {
        m_yearsales = sales;
    } else {
        QJsonObject data;
        data["receiptNum"] = id;
        data["receiptTime"] = to.toString(Qt::ISODate);

        query.prepare("UPDATE receipts SET gross=:gross, timestamp=:timestamp, infodate=:infodate WHERE receiptNum=:receiptNum");
        query.bindValue(":gross", gross.toDouble());
        query.bindValue(":timestamp", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.bindValue(":infodate", to.toString(Qt::ISODate));
        query.bindValue(":receiptNum", id);

        bool ok = query.exec();

        if (!ok) {
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
        }
    }

    stat.append(QString("%1: %2").arg(type).arg(sales));
    stat.append("=");

    if (Database::isAnyValueFunctionAvailable())
        query.prepare("SELECT sum(orders.count) AS count, products.name, orders.gross, SUM((orders.count * orders.gross) - ((orders.count * orders.gross / 100) * orders.discount)) as total, ANY_VALUE(orders.tax) as tax, orders.discount FROM orders LEFT JOIN products ON orders.product=products.id  LEFT JOIN receipts ON receipts.receiptNum=orders.receiptId WHERE receipts.timestamp BETWEEN :fromDate AND :toDate AND receipts.payedBy < 3 GROUP BY products.name, orders.gross, orders.discount ORDER BY tax, products.name ASC");
    else
        query.prepare("SELECT sum(orders.count) AS count, products.name, orders.gross, SUM((orders.count * orders.gross) - ((orders.count * orders.gross / 100) * orders.discount)) as total, orders.tax, orders.discount FROM orders LEFT JOIN products ON orders.product=products.id  LEFT JOIN receipts ON receipts.receiptNum=orders.receiptId WHERE receipts.timestamp BETWEEN :fromDate AND :toDate AND receipts.payedBy < 3 GROUP BY products.name, orders.gross, orders.discount ORDER BY orders.tax, products.name ASC");

    query.bindValue(":fromDate", from.toString(Qt::ISODate));
    query.bindValue(":toDate", to.toString(Qt::ISODate));

    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    stat.append(tr("Verkaufte Artikel oder Leistungen (Gruppiert) Gesamt %1").arg(sumProducts.toString().replace(".",",")));
    while (query.next())
    {
        QString name;
        if (query.value("discount").toDouble() != 0.0) {
            QBCMath discount(query.value("discount").toDouble());
            name = QString("%1 (Rabatt -%2%)").arg(query.value("name").toString()).arg(QBCMath::bcround(discount.toString(), 2).replace(".",","));
        } else {
            name = query.value("name").toString();
        }

        QBCMath total(query.value("total").toDouble());
        total.round(2);
        QBCMath gross(query.value("gross").toDouble());
        gross.round(2);
        QBCMath tax(query.value("tax").toDouble());
        tax.round(2);

        stat.append(QString("%1: %2: %3: %4: %5%")
                    .arg(query.value("count").toString())
                    .arg(name)
                    .arg(gross.toString().replace(".",","))
                    .arg(total.toString().replace(".",","))
                    .arg(tax.toString().replace(".",",")));
    }

    return stat;
}

/**
 * @brief Reports::insert
 * @param list
 * @param id
 */
bool Reports::insert(QStringList list, int id, QDateTime to)
{

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    int count = list.count();
    Spread::Instance()->setProgressBarValue(0);

    int i=0;
    bool ret = true;
    Journal journal;
    foreach (QString line, list) {
        journal.journalInsertLine("Textposition", line);
        query.prepare("INSERT INTO reports (receiptNum, timestamp, text) VALUES(:receiptNum, :timestamp, :text)");
        query.bindValue(":receiptNum", id);
        query.bindValue(":timestamp", to.toString(Qt::ISODate));
        query.bindValue(":text", line);

        ret = query.exec();
        if (!ret) {
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
            break;
        }

        Spread::Instance()->setProgressBarValue(((float)i++ / (float)count) * 100 );
    }
    return ret;
}

/**
 * @brief Reports::createYearStat
 * @param id
 * @param date
 * @return
 */
QStringList Reports::createYearStat(int id, QDate date)
{
    QDateTime from;
    QDateTime to;

    // ----------- YEAR ---------------------------
    QStringList eoy;
    QString fromString = QString("%1-01-01").arg(date.year());
    from.setDate(QDate::fromString(fromString, "yyyy-MM-dd"));
    to.setDate(QDate::fromString(date.toString()));
    to.setTime(QTime::fromString("23:59:59"));

    eoy.append(QString("Jahressummen %1:").arg(date.year()));
    eoy.append("-");

    eoy.append(createStat(id, "Jahresumsatz", from, to));

    return eoy;
}

/**
 * @brief Reports::getReport
 * @param id
 * @param test
 * @return
 */
QString Reports::getReport(int id, bool test)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT receipts.payedBy, reports.timestamp FROM receipts JOIN reports ON receipts.receiptNum=reports.receiptNum WHERE receipts.receiptNum=:id");

    if (test)
        query.prepare("SELECT receipts.payedBy, reports.timestamp FROM receipts JOIN reports ON receipts.receiptNum=reports.receiptNum WHERE receipts.receiptNum=(SELECT min(receiptNum) FROM reports)");
    else
        query.bindValue(":id", id);

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.next();

    int type = query.value("payedBy").toInt();
    QString format = (type == PAYED_BY_REPORT_EOM)? "MMMM yyyy": "dd MMMM yyyy";

    QString header;
    if (test)
        header = QString("TESTDRUCK für SCHRIFTART");
    else
        header = QString("BON # %1, %2 - %3").arg(id).arg(Database::getActionType(type)).arg(query.value("timestamp").toDate().toString(format));

    query.prepare("SELECT text FROM reports WHERE receiptNum=:id");

    if (test)
        query.prepare("SELECT text FROM reports WHERE receiptNum=(SELECT min(receiptNum) FROM reports)");
    else
        query.bindValue(":id", id);

    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    QString text;

    text.append("<!DOCTYPE html><html><head>\n");
    text.append("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n");
    text.append("</head><body>\n<table cellpadding=\"3\" cellspacing=\"1\" width=\"100%\">\n");

    int x = 0;
    int span = 5;
    bool needOneMoreCol = false;

    text.append(QString("<tr><th colspan=\"%1\">%2</th></tr>").arg(span).arg(header) );
    text.append(QString("<tr><th colspan=\"%1\"></th></tr>").arg(span));

    while (query.next()){
        needOneMoreCol = false;
        span = 5;

        QString t = query.value("text").toString();
        x++;
        QString color = "";
        if (x % 2 == 1)
            color = "bgcolor='#F5F5F5'";
        else
            color = "bgcolor='#FFFFFF'";

        text.append("<tr>");

        QStringList list;

        if (t.indexOf('-') == 0 && t.size() == 1) {
            t.replace('-',"<hr>");
            text.append(QString("<td colspan=\"%1\" %2>%3</td>").arg(span).arg(color).arg(t));
            needOneMoreCol = false;
        } else if (t.indexOf('=') == 0  && t.size() == 1) {
            t.replace('=',"<hr size=\"5\">");
            text.append(QString("<td colspan=\"%1\" %2>%3</td>").arg(span).arg(color).arg(t));
            needOneMoreCol = false;
        } else if (t.indexOf(QRegularExpression("[0-9]{1,2}%:")) != -1) {
            list = t.split(":");
            span = span - list.count();
            foreach (const QString &str, list) {
                if (test)
                    text.append(QString("<td align=\"right\" colspan=\"%1\" %2>%3</td>").arg(span).arg(color).arg("0,00"));
                else
                    text.append(QString("<td align=\"right\" colspan=\"%1\" %2>%3</td>").arg(span).arg(color).arg(str));
                span = 1;
            }
            needOneMoreCol = true;
        } else if (t.indexOf(QRegularExpression("^-*\\d+:")) != -1) {
            list = t.split(": ",QString::SkipEmptyParts);
            span = span - list.count();
            int count = 0;

            QString align = "left";
            foreach (const QString &str, list) {
                if (count > 1) align="right";
                if (test && count > 1)
                    text.append(QString("<td align=\"%1\" colspan=\"%2\" %3>%4</td>").arg(align).arg(span).arg(color).arg("0,00"));
                else
                    text.append(QString("<td align=\"%1\" colspan=\"%2\" %3>%4</td>").arg(align).arg(span).arg(color).arg(str));

                count++;
                span = 1;
            }
            needOneMoreCol = false;
        } else {
            list = t.split(":",QString::SkipEmptyParts);
            span = span - list.count();
            int count = 0;

            QString align = "left";
            foreach (const QString &str, list) {
                if (count > 0) align="right";
                if (test && count > 0)
                    text.append(QString("<td align=\"%1\" colspan=\"%2\" %3>%4</td>").arg(align).arg(span).arg(color).arg("0,00"));
                else
                    text.append(QString("<td align=\"%1\" colspan=\"%2\" %3>%4</td>").arg(align).arg(span).arg(color).arg(str));

                count++;
                span = 1;
            }
            needOneMoreCol = true;
        }

        if (needOneMoreCol)
            text.append(QString("<td colspan=\"%1\" %2></td>").arg(span).arg(color));

        text.append("</tr>");
    }
    text.append("</table></body></html>\n");

    return text;
}

void Reports::printDocument(int id, QString title)
{
    QString DocumentTitle = QString("BON_%1_%2").arg(id).arg(title);
    QTextDocument doc;
    doc.setHtml(Reports::getReport(id));
    DocumentPrinter p;
    p.printDocument(&doc, DocumentTitle);
}
