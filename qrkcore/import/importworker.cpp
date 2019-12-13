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

#include "importworker.h"
#include "singleton/spreadsignal.h"
#include "preferences/qrksettings.h"
#include "RK/rk_signaturemodule.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "database.h"
#include "databasemanager.h"
#include "documentprinter.h"
#include "utils/utils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QQueue>
#include <QThread>
#include <QWidget>
#include <QTextCodec>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

ImportWorker::ImportWorker(QQueue<QString> &queue, QWidget *parent)
    :Reports(parent, true)
{
    m_queue = &queue;
    m_isStopped = false;
    connect(this, &ImportWorker::not_a_number, this, &ImportWorker::number_error);
}

ImportWorker::ImportWorker(QWidget *parent)
    :Reports(parent, true)
{
    m_isStopped = false;
    connect(this, &ImportWorker::not_a_number, this, &ImportWorker::number_error);
}

ImportWorker::~ImportWorker()
{
    {
        // QSqlDatabase dbc = Database::database();
        qDebug() << "Function Name: " << Q_FUNC_INFO << " Destructor from Worker thread: " << QThread::currentThread();
        disconnect(this, &ImportWorker::not_a_number, Q_NULLPTR, Q_NULLPTR);
        /*
        if (dbc.driverName() == "QSQLITE") {
            QSqlQuery query(dbc);
            query.exec("PRAGMA wal_checkpoint;");
            query.next();
            qDebug() << "Function Name: " << Q_FUNC_INFO << "WAL Checkpoint: (busy:" << query.value(0).toString() << ") log: " << query.value(1).toString() << " checkpointed: " << query.value(2).toString();
        }
        */
    }
    DatabaseManager::removeCurrentThread("CN");
}

void ImportWorker::stopProcess()
{
    m_isStopped = true;
}

void ImportWorker::process()
{
    while (!m_isStopped && !m_queue->isEmpty()) {
        qDebug() << "Function Name: " << Q_FUNC_INFO << " From Worker thread: " << QThread::currentThread();

        if (checkEOAny()) {
            QString filename = m_queue->first();
            loadJSonFile(filename);
            QThread::msleep(200);
            m_queue->dequeue();
        } else {
            int qsize = m_queue->size();
            while (!m_isStopped && !m_queue->isEmpty())
                fileMover(m_queue->dequeue(), ".false");

            QString info;
            if (!RKSignatureModule::isSignatureModuleSetDamaged())
                info = tr("Import Fehler -> Tages/Monatsabschluss wurde schon erstellt. Es wurden %1 Dateien umbenannt.").arg(qsize);
            else
                info = tr("Import Fehler -> Es wurden %1 Dateien umbenannt. (siehe Logdatei)").arg(qsize);
            Spread::Instance()->setImportInfo(info, true);
            break;
        }
    }
    emit finished();
}

void ImportWorker::number_error(QString what)
{
    Spread::Instance()->setImportInfo(tr("Import Fehler JSON Datenformat von %1 is keine Zahl").arg(what), true);
}

void ImportWorker::database_error(QString what)
{
    Spread::Instance()->setImportInfo(what, true);
}

bool ImportWorker::loadJSonFile(QString filename)
{

    QByteArray receiptInfo;
    QFile file(filename);

    // in some cases we are to fast here. Try 3 times to open the file while file is in use by create process
    for (int i = 0; i < 3; i++) {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            break;
        } else if (i == 3) {
            Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht geöffnet werden.").arg(filename), true);
            return false;
        }
        QThread::msleep(300);
    }

    receiptInfo = file.readAll();

    file.close();
    return processJson(receiptInfo, file.fileName());
}

bool ImportWorker::processJson(QByteArray receiptInfo, QString filename)
{
    // remove any prefix and suffix from dirty json files
    if (!receiptInfo.startsWith('{')) {
        int begin = receiptInfo.indexOf('{');
        int end = receiptInfo.lastIndexOf('}') - begin + 1;
        receiptInfo = receiptInfo.mid(begin,end);
    }

    if (receiptInfo.isEmpty()) {
        Spread::Instance()->setImportInfo(tr("INFO: Import %1 wurde ignoriert -> Keine JSON Datei.").arg(filename),true);
        fileMover(filename, ".false");
        return false;
    }

    QrkSettings settings;
    QJsonParseError jerror;

    QTextCodec *codec = QTextCodec::codecForName (settings.value("importCodePage", "UTF-8").toString().toUtf8());
    QString json = codec->toUnicode(receiptInfo);
    QJsonDocument jd = QJsonDocument::fromJson(json.toUtf8(), &jerror);
    QJsonObject data = jd.object();

    if (data.contains("r2b")) {
        data["filename"] = filename;
        if (importR2B(data)) {
            Spread::Instance()->setImportInfo(tr("Import %1 -> OK").arg(filename));
            if (!fileMover(filename, ".old")) {
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden").arg(filename), true);
                return false;
            }
            return true;

        } else {
            Spread::Instance()->setImportInfo(tr("Import Fehler -> Dateiname: %1.").arg(filename), true);
            fileMover(filename, ".false");
            return false;
        }

    } else if (data.contains("receipt")) {
        data["filename"] = filename;
        if (importReceipt(data)) {
            Spread::Instance()->setImportInfo(tr("Import %1 -> OK").arg(filename));
            if (!fileMover(filename, ".old")) {
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(filename), true);
                return false;
            }
            return true;

        } else {
            if (!fileMover(filename, ".false"))
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(filename), true);
            Spread::Instance()->setImportInfo(tr("Import %1 -> Fehler").arg(data.value("filename").toString()), true);
        }

    } else if (data.contains("printtagged")) {
        data["filename"] = filename;
        if (importTagged(data)) {
            Spread::Instance()->setImportInfo(tr("Import %1 -> Druck OK").arg(filename));
            if (!fileMover(filename, ".old"))
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(filename), true);

            return true;
        }
        if (!fileMover(filename, ".false"))
            Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(filename), true);

        Spread::Instance()->setImportInfo(tr("Import %1 -> Fehler").arg(filename), true);
        return false;

    } else {
        data["filename"] = filename;
        if (importAny(data)) {
            Spread::Instance()->setImportInfo(tr("Import %1 -> OK").arg(filename));
            if (!fileMover(filename, ".old")) {
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(filename), true);
                return false;
            }
            if (!fileMover(filename, ".old")) {
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(filename), true);
                return false;
            }
            return true;
        }
        Spread::Instance()->setImportInfo(tr("Import Fehler -> %1 [Offset: %2] (%3)").arg(jerror.errorString()).arg(jerror.offset).arg(filename), true);
        fileMover(filename, ".false");
    }

    return false;
}

bool ImportWorker::importAny(QJsonObject data)
{

    QJsonObject root;
    bool cb = data.contains("zahlungsmittel") && data.contains("positionen");
    if (cb) {
        Spread::Instance()->setImportInfo(tr("Versuche cBird Json Daten zu importieren!"));
        QString payedBy = data.value("zahlungsmittel").toString().replace("Bar", "0").replace("Bankomat", "1").replace("Kreditkarte", "2");
        QJsonArray cbArray = data.value("positionen").toArray();
        QJsonObject anyData;
        QJsonArray anyArray;
        QJsonArray rootArray;
        anyData["payedBy"] = payedBy;
        foreach (const QJsonValue &value, cbArray) {
            QJsonObject jsonItem = value.toObject();
            QJsonObject anyItem;
            if (jsonItem.contains("menge")) {
                QBCMath gross = jsonItem.value("einzelpreis").toInt();
                QBCMath count = jsonItem.value("menge").toInt();
                QBCMath tax = jsonItem.value("ust").toInt();

                gross = gross / 100;
                gross.round(2);

                anyItem["name"] = jsonItem.value("bezeichnung").toString();
                anyItem["count"] = count.toString();
                anyItem["tax"] = tax.toString();
                anyItem["gross"] = gross.toString();
                anyArray.append(anyItem);
            }
        }
        anyData["items"] = anyArray;
        rootArray.append(anyData);
        root["receipt"] = rootArray;
        return ImportWorker::importReceipt(root);
    }
    return false;
}

bool ImportWorker::importR2B(QJsonObject data)
{
    QJsonArray r2bArray = data.value("r2b").toArray();
    bool ok = false;

    QSqlDatabase dbc = Database::database();
    ok = dbc.transaction();
    m_transaction++;
    qDebug() << "Function Name: " << Q_FUNC_INFO << " transaction start: " << m_transaction;;
    if (!ok) {
        emit database_error(QString("Transaction failed(%1), %2 %3").arg(ok).arg(dbc.lastError().text()).arg(dbc.lastError().nativeErrorCode()));
        return ok;
    }

    foreach (const QJsonValue & value, r2bArray) {
        QJsonObject obj = value.toObject();
        ok = obj.contains("gross") && obj.contains("receiptNum") && obj.contains("payedBy");
        if (ok) {
//            ok = false;
            newOrder();
            if (! setR2BServerMode(obj)) {
                QString info = tr("Import Fehler -> Rechnungsnummer: %1 aus Importdatei %2 wird schon verwendet!").arg(obj.value("receiptNum").toString()).arg(data.value("filename").toString());
                Spread::Instance()->setImportInfo(info, true);
                ok = false;
            } else if (int id = createReceipts()) {
                setCurrentReceiptNum(id);
                if (createOrder()) {
                    if (finishReceipts(obj.value("payedBy").toString().toInt())) {
                        ok = dbc.commit();
                        qDebug() << "Function Name: " << Q_FUNC_INFO << " transaction commit: " << m_transaction;
                        m_transaction--;
                    } else {ok = false;}
                } else {ok = false;}
            }
        } else {
            QString info = tr("Import Fehler -> Falsches JSON Format, Dateiname: %1").arg(data.value("filename").toString());
            Spread::Instance()->setImportInfo(info, true);
            ok = false;
        }
    }

    if (!ok) {
        bool sql_ok = dbc.rollback();
        qDebug() << "Function Name: " << Q_FUNC_INFO << " transaction rollback: " << m_transaction;
        m_transaction--;
        emit database_error(QString("Rollback = %1,%2 %3").arg(sql_ok).arg(dbc.lastError().text()).arg(dbc.lastError().nativeErrorCode()));
    }

    return ok;
}

bool ImportWorker::importReceipt(QJsonObject data)
{
    QJsonArray receiptData = data.value("receipt").toArray();
    bool ok = false;

    QSqlDatabase dbc = Database::database();
    ok = dbc.transaction();
    m_transaction++;
    qDebug() << "Function Name: " << Q_FUNC_INFO << " transaction start: " << m_transaction;
    if (!ok) {
        emit database_error(QString("Transaction failed(%1), %2 %3").arg(ok).arg(dbc.lastError().text()).arg(dbc.lastError().nativeErrorCode()));
        return ok;
    }

    foreach (const QJsonValue & value, receiptData) {
        QJsonObject obj = value.toObject();
        ok = obj.contains("payedBy") && obj.contains("items");
        if (ok) {
            ok = false;
            newOrder();
            if (! setReceiptServerMode(obj)) {
                QString info = tr("Import Fehler -> Importdatei %1!").arg(data.value("filename").toString());
                Spread::Instance()->setImportInfo(info, true);
                ok = false;
            } else if (int id = createReceipts()) {
                setCurrentReceiptNum(id);
                if (createOrder()) {
                    if (finishReceipts(obj.value("payedBy").toString().toInt())) {
                        ok = dbc.commit();
                        qDebug() << "Function Name: " << Q_FUNC_INFO << " transaction commit: " << m_transaction;
                        m_transaction--;
                    } else {ok = false;}
                } else {ok = false;}
            }
        } else {
            QString info = tr("Import Fehler -> Falsches JSON Format, Dateiname: %1").arg(data.value("filename").toString());
            Spread::Instance()->setImportInfo(info, true);
            ok = false;
        }
    }
    if (!ok) {
        bool sql_ok = dbc.rollback();
        qDebug() << "Function Name: " << Q_FUNC_INFO << " transaction rollback: " << m_transaction;
        m_transaction--;
        emit database_error(QString("Rollback = %1,%2 %3").arg(sql_ok).arg(dbc.lastError().text()).arg(dbc.lastError().nativeErrorCode()));
    }

    return ok;
}

bool ImportWorker::importTagged(QJsonObject data)
{
    QJsonArray taggedData = data.value("printtagged").toArray();
    bool ok = false;

    foreach (const QJsonValue & value, taggedData) {
        QJsonObject obj = value.toObject();
        ok = obj.contains("customerText") && obj.contains("printer") && obj.contains("items");
        if (ok) {
            DocumentPrinter p;
            p.printTagged(obj);
        } else {
            QString info = tr("Import Fehler -> Falsches JSON Format, Dateiname: %1").arg(data.value("filename").toString());
            Spread::Instance()->setImportInfo(info, true);
            ok = false;
        }
    }

    return ok;
}

bool ImportWorker::fileMover(QString filename, QString ext)
{
    qDebug() << "Function Name: " << Q_FUNC_INFO << " filename: " << filename << " ext: " << ext;
    QString originalfilename = filename;

    QFileInfo ofi(originalfilename);
    if (!ofi.exists())
        return true;

    QString original_ext = "." + ofi.suffix();

    QFileInfo fi(filename.replace(original_ext, ext));
    QDir directory(fi.absoluteDir());
    QStringList filter;
    filter.append(fi.baseName() + ext + "*");
    QFileInfoList fIL;
    fIL = directory.entryInfoList(filter, QDir::Files, QDir::Time);

    QStringList entryList = directory.entryList(filter);

    std::sort(
        entryList.begin(),
        entryList.end(), Utils::compareNames);

    if (entryList.size()>0) {
        QString last = entryList.last();
        QString e = last.section(".", -1,-1);
        if (e == ext.section(".", -1,-1)) {
            ext = ".001";
        } else {
            uint i = e.toUInt() +1;
            if (i+1 == 0) {
                QString info = tr("ACHTUNG -> Maximaler Datenzähler erreicht, Dateiendung: %1").arg(i);
                Spread::Instance()->setImportInfo(info, true);
            }

            if (e.size() > 4)
                ext = "." + QString::number(i);
            else
                ext = "." + QString("%1").arg(i, e.size(), 'g', -1, '0');

            if (i > 99) {
                Spread::Instance()->setImportInfo(tr("INFO: Dateiname nicht unterstützt. Nähere Infos: https://www.ckvsoft.at"), true);
                Spread::Instance()->setImportInfo(tr("INFO: Verwenden Sie fortlaufende Nummern in Ihren Dateinamen. (zB. BON001.json, BON002.json ... BON010.json)"), true);
                qInfo() << "Function Name: " << Q_FUNC_INFO << "unsupported Filename: " << originalfilename << " backupname: " << filename << ext;
            }
        }

        QFile f(originalfilename);
        bool success = f.rename(filename + ext);
        if (!success) emit apport(false);

        return success;
    }

    QFile f(originalfilename);

    bool success = f.rename(filename);
    if (!success) emit apport(false);

    return success;
}
