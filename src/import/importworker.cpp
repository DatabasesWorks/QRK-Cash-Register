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
#include "database.h"
#include "databasemanager.h"
#include "documentprinter.h"

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

ImportWorker::~ImportWorker()
{
    qDebug() << "Function Name: " << Q_FUNC_INFO << " Destructor from Worker thread: " << QThread::currentThread();
    disconnect(this, &ImportWorker::not_a_number, 0, 0);
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

        if (checkEOAnyServerMode()) {
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
        }
        if (i == 3) {
            Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht geöffnet werden.").arg(filename), true);
            return false;
        }
        QThread::msleep(300);
    }

    receiptInfo = file.readAll();

    file.close();
    // remove any prefix and suffix from dirty json files
    if (!receiptInfo.startsWith('{')) {
        int begin = receiptInfo.indexOf('{');
        int end = receiptInfo.lastIndexOf('}') - begin + 1;
        receiptInfo = receiptInfo.mid(begin,end);
    }

    QrkSettings settings;
    QJsonParseError jerror;

    QTextCodec *codec = QTextCodec::codecForName (settings.value("importCodePage", "UTF-8").toString().toUtf8());
    QString json = codec->toUnicode(receiptInfo);
    QJsonDocument jd = QJsonDocument::fromJson(json.toUtf8(), &jerror);
    QJsonObject data = jd.object();

    if (data.contains("r2b")) {
        data["filename"] = file.fileName();
        if (importR2B(data)) {
            Spread::Instance()->setImportInfo(tr("Import %1 -> OK").arg(data.value("filename").toString()));
            if (!fileMover(filename, ".old"))
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden").arg(data.value("filename").toString()), true);

            return true;

        } else {
            Spread::Instance()->setImportInfo(tr("Import Fehler -> Falsches Dateiformat (%1).").arg(filename), true);
            fileMover(filename, ".false");
            return false;
        }

    } else if (data.contains("receipt")) {
        data["filename"] = file.fileName();
        if (importReceipt(data)) {
            Spread::Instance()->setImportInfo(tr("Import %1 -> OK").arg(data.value("filename").toString()));
            if (!fileMover(filename, ".old")) {
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(data.value("filename").toString()), true);
                return false;
            }
            return true;

        } else {
            if (!fileMover(filename, ".false"))
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(data.value("filename").toString()), true);
            Spread::Instance()->setImportInfo(tr("Import %1 -> Fehler").arg(data.value("filename").toString()), true);
        }

    } else if (data.contains("printtagged")) {
        data["filename"] = file.fileName();
        if (importTagged(data)) {
            Spread::Instance()->setImportInfo(tr("Import %1 -> Druck OK").arg(data.value("filename").toString()));
            if (!fileMover(filename, ".old"))
                Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(data.value("filename").toString()), true);

            return true;
        }
        if (!fileMover(filename, ".false"))
            Spread::Instance()->setImportInfo(tr("Import Fehler -> Datei %1 kann nicht umbenannt werden.").arg(data.value("filename").toString()), true);

        Spread::Instance()->setImportInfo(tr("Import %1 -> Fehler").arg(data.value("filename").toString()), true);
        return false;

    } else {
        Spread::Instance()->setImportInfo(tr("Import Fehler -> %1 [Offset: %2] (%3)").arg(jerror.errorString()).arg(jerror.offset).arg(filename), true);
        fileMover(filename, ".false");
    }

    return false;
}

bool ImportWorker::importR2B(QJsonObject data)
{
    QJsonArray r2bArray = data.value("r2b").toArray();
    bool ok = false;

    QSqlDatabase dbc = Database::database();
    ok = dbc.transaction();
    if (!ok) {
        emit database_error(QString("Transaction failed(%1), %2 %3").arg(ok).arg(dbc.lastError().text()).arg(dbc.lastError().nativeErrorCode()));
        return ok;
    }

    foreach (const QJsonValue & value, r2bArray) {
        QJsonObject obj = value.toObject();
        ok = obj.contains("gross") && obj.contains("receiptNum") && obj.contains("payedBy");
        if (ok) {
            ok = false;
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
        emit database_error(QString("Rollback = %1,%2 %3").arg(sql_ok).arg(dbc.lastError().text()).arg(dbc.lastError().nativeErrorCode()));
    }

    if (dbc.driverName() == "QSQLITE") {
        QSqlQuery query(dbc);
        query.exec("PRAGMA wal_checkpoint;");
        query.next();
        qDebug() << "Function Name: " << Q_FUNC_INFO << "WAL Checkpoint: (busy:" << query.value(0).toString() << ") log: " << query.value(1).toString() << " checkpointed: " << query.value(2).toString();
    }
    return ok;
}

bool ImportWorker::importReceipt(QJsonObject data)
{
    QJsonArray receiptData = data.value("receipt").toArray();
    bool ok = false;

    QSqlDatabase dbc = Database::database();
    ok = dbc.transaction();
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
        emit database_error(QString("Rollback = %1,%2 %3").arg(sql_ok).arg(dbc.lastError().text()).arg(dbc.lastError().nativeErrorCode()));
    }

    if (dbc.driverName() == "QSQLITE") {
        QSqlQuery query(dbc);
        query.exec("PRAGMA wal_checkpoint;");
        query.next();
        qDebug() << "Function Name: " << Q_FUNC_INFO << "WAL Checkpoint: (busy:" << query.value(0).toString() << ") log: " << query.value(1).toString() << " checkpointed: " << query.value(2).toString();
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
    QString original_ext = "." + ofi.suffix();

    QFileInfo fi(filename.replace(original_ext, ext));
    QDir directory(fi.absoluteDir());
    QStringList filter;
    filter.append(fi.baseName() + ext + "*");
    QFileInfoList fIL;
    fIL = directory.entryInfoList(filter, QDir::Files, QDir::Time);

    if (fIL.size()>0) {
        QFileInfo fi = fIL.first();
        QString e = fi.fileName().section(".", -1,-1);
        if (e == ext.section(".", -1,-1)) {
            ext = ".001";
        } else {
            int i = e.toInt() +1;
            ext = "." + QString("%1").arg(i, e.size(), 'g', -1, '0');;
        }
        QFile f(originalfilename);
        return f.rename(filename + ext);
    }
    QFile f(originalfilename);

    return f.rename(filename);
}
