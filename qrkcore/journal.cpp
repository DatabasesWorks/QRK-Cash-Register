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

#include "journal.h"
#include "defines.h"
#include "database.h"
#include "preferences/qrksettings.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "3rdparty/ckvsoft/rbac/crypto.h"
#include "QCryptographicHash"
#include "bcmath.h"
#include "qrkprogress.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QApplication>
#include <QDebug>

Journal::Journal(QObject *parent)
  : QObject(parent)
{
}

void Journal::journalInsertReceipt(QJsonObject &data)
{
  QSqlDatabase dbc = Database::database();
  QSqlQuery query(dbc);

  QrkSettings settings;
  int digits = settings.value("decimalDigits", 2).toInt();;

  // Id Programmversion Kassen-Id Beleg Belegtyp Bemerkung Nachbonierung
  // Belegnummer Datum Umsatz_Normal Umsatz_Ermaessigt1 Umsatz_Ermaessigt2
  // Umsatz_Null Umsatz_Besonders Jahresumsatz_bisher Erstellungsdatum

  QString var;
  QString val = "(version,cashregisterid,datetime,data,checksum,userId)";

  QJsonArray a = data.value("Orders").toArray();

  foreach (const QJsonValue & value, a) {
    var.clear();
    QJsonObject o = value.toObject();
    var.append(QString("Produktposition\t"));
    QString itemNum = o["itemNum"].toString();
    if (!itemNum.isEmpty())
        itemNum = QString("%1 ").arg(itemNum);

    if (o["discount"].toDouble() != 0.0)
        var.append(QString("%1%2 Rabatt: -%3%\t").arg(itemNum).arg(o["product"].toString()).arg(QString::number(o["discount"].toDouble(),'f', 2)).replace(".",","));
    else
        var.append(QString("%1 %2\t").arg(itemNum).arg(o["product"].toString()));
    var.append(QString("%1\t").arg(QString::number(o["count"].toDouble(),'f', digits)).replace(".",","));
    var.append(QString("%1\t").arg(QString::number(o["singleprice"].toDouble(),'f', 2)).replace(".",","));
    var.append(QString("%1\t").arg(QString::number(o["gross"].toDouble() - (o["gross"].toDouble() / 100) * o["discount"].toDouble(),'f', 2)).replace(".",","));
    var.append(QString("%1\t").arg(o["tax"].toDouble()).replace(".",","));
    var.append(QString("%1").arg(data.value("receiptTime").toString()));

    SecureByteArray sa = var.toUtf8();
    QString textdata = Crypto::encrypt(sa, SecureByteArray("Journal"));
    QString checksum = QCryptographicHash::hash(textdata.toUtf8(), QCryptographicHash::Sha1).toHex().toUpper();

    bool ok = query.prepare(QString("INSERT INTO journal %1 VALUES(:version,:kasse,:receiptTime,:data,:checksum,:userId)")
        .arg(val));

    if (!ok) {
      qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
      qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
    }

    query.bindValue(":version", data.value("version").toString());
    query.bindValue(":kasse", data.value("kasse").toString());
    query.bindValue(":receiptTime", data.value("receiptTime").toString());
    query.bindValue(":data", textdata);
    query.bindValue(":checksum", checksum);
    query.bindValue(":userId", RBAC::Instance()->getUserId());

    if (!ok) {
      qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
      qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
    }

    query.exec();

  }

  var.clear();
  var.append(QString("%1\t").arg(data.value("actionText").toString()));
  var.append(QString("%1\t").arg(data.value("typeText").toString()));
  var.append(QString("%1\t").arg(data.value("comment").toString()));
  var.append(QString("%1\t").arg(data.value("totallyup").toString()));
  var.append(QString("%1\t").arg(data.value("receiptNum").toInt()));
  var.append(QString("%1\t").arg(data.value("receiptTime").toString()));
  var.append(QString("%1\t").arg(QString::number(data.value("Satz-Normal").toDouble(),'f',2)).replace(".",","));
  var.append(QString("%1\t").arg(QString::number(data.value("Satz-Ermaessigt-1").toDouble(),'f',2)).replace(".",","));
  var.append(QString("%1\t").arg(QString::number(data.value("Satz-Ermaessigt-2").toDouble(),'f',2)).replace(".",","));
  var.append(QString("%1\t").arg(QString::number(data.value("Satz-Null").toDouble(),'f',2)).replace(".",","));
  var.append(QString("%1\t").arg(QString::number(data.value("Satz-Besonders").toDouble(),'f',2)).replace(".",","));
  var.append(QString("%1\t").arg(QString::number(data.value("sumYear").toDouble(),'f',2)).replace(".",","));
  var.append(QString("%1").arg(data.value("receiptTime").toString()));

  SecureByteArray sa = var.toUtf8();
  QString textdata = Crypto::encrypt(sa, SecureByteArray("Journal"));
  QString checksum = QCryptographicHash::hash(textdata.toUtf8(), QCryptographicHash::Sha1).toHex().toUpper();

  bool ok = query.prepare(QString("INSERT INTO journal %1 VALUES(:version,:kasse,:date,:data,:checksum,:userId)")
      .arg(val));

  if (!ok) {
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
  }

  query.bindValue(":version", data.value("version").toString());
  query.bindValue(":kasse", data.value("kasse").toString());
  query.bindValue(":date", QDateTime::currentDateTime().toString(Qt::ISODate));
  query.bindValue(":data", textdata);
  query.bindValue(":checksum", checksum);
  query.bindValue(":userId", RBAC::Instance()->getUserId());

  ok = query.exec();
  if (!ok) {
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
  }
}

void Journal::journalInsertLine(QString title,  QString text)
{

  QDateTime dt = QDateTime::currentDateTime();
  QSqlDatabase dbc = Database::database();
  QSqlQuery query(dbc);
  QString val = "(version,cashregisterid,datetime,data,checksum,userId)";
  bool ok = query.prepare(QString("INSERT INTO journal %1 VALUES(:version, :kasse, :date, :data, :checksum, :userId)")
                      .arg(val));

  if (!ok) {
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
  }

  SecureByteArray sa = QString(title + "\t" + text + "\t" + dt.toString(Qt::ISODate)).toUtf8();
  QString data = Crypto::encrypt(sa, SecureByteArray("Journal"));
  QString checksum = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex().toUpper();

  query.bindValue(":version", QString("%1.%2").arg(QRK_VERSION_MAJOR).arg(QRK_VERSION_MINOR));
  query.bindValue(":kasse", Database::getCashRegisterId());
  query.bindValue(":date", dt.toString(Qt::ISODate));
  query.bindValue(":data", data);
  query.bindValue(":checksum", checksum);
  query.bindValue(":userId", RBAC::Instance()->getUserId());

  ok = query.exec();

  if (!ok) {
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
  }
}

void Journal::encodeJournal(QSqlDatabase dbc)
{
    QSqlQuery query(dbc);
    QSqlQuery query2(dbc);

    query.exec("SELECT checksum FROM journal");
    if (query.next())
        return;

    QRKProgress progress;
    progress.setText(tr("Datenbank Update"));
    progress.show();
    dbc.transaction();

    query.exec("ALTER TABLE journal RENAME TO journal_backup");

    QString createQuery = "";
    if (dbc.driverName() == "QMYSQL") {
        createQuery = "CREATE TABLE `journal` ("
                      "`id` int(11) NOT NULL AUTO_INCREMENT,"
                      "`version` text NOT NULL,"
                      "`cashregisterid` text NOT NULL,"
                      "`datetime` datetime NOT NULL,"
                      "`data` text,"
                      "`checksum` text,"
                      "`userId` int(11) NOT NULL DEFAULT '0',"
                      "PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=utf8;";
    } else {
        createQuery = "CREATE TABLE `journal` ("
                      "`id`                INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                      "`version`           text NOT NULL,"
                      "`cashregisterid`    text NOT NULL,"
                      "`datetime`          datetime NOT NULL,"
                      "`data`              text,"
                      "`checksum`          text,"
                      "`userId`            INTEGER NOT NULL DEFAULT '0'"
                      ");";
    }

    if (query2.prepare(createQuery))
        query2.exec();

    query.prepare(QString("SELECT id, version, cashregisterid, datetime, text, userid FROM journal_backup WHERE id < 5"));

    query.exec();

    int numRows = query.numRowsAffected();

    query2.prepare(QString("INSERT INTO journal (id, version, cashregisterid, datetime, data, checksum, userid) VALUES(:id, :version, :cashregisterid, :datetime, :data, :checksum, :userid)"));
    while (query.next())
    {
        SecureByteArray sa = query.value("text").toString().toUtf8();
        QString text = Crypto::encrypt(sa, SecureByteArray("Journal"));
        QString checksum = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex().toUpper();

        query2.bindValue(":id", query.value("id").toInt());
        query2.bindValue(":version",query.value("version").toString());
        query2.bindValue(":cashregisterid", query.value("cashregisterid").toString());
        query2.bindValue(":datetime", query.value("datetime").toString());
        query2.bindValue(":data", text);
        query2.bindValue(":checksum", checksum);
        query2.bindValue(":userid", query.value("userid").toInt());
        query2.exec();
    }

    query.prepare(QString("SELECT id, version, cashregisterid, datetime, text, userid FROM journal_backup WHERE id > 4"));

    query.exec();
    int i = 0;
    QBCMath sum = 0;
    while (query.next())
    {
        i++;
        progress.progress(((float)i / (float)numRows) * 100);
        qApp->processEvents();

        QString text = query.value("text").toString();
        QStringList list = text.split('\t');
        QBCMath qsum = 0;

        if (list.count() == 7) {
            if (list.at(0) == "Produktposition") {
                for (int i = 2;i < 6; i++) {
                    QString s = list.at(i);
                    list[i] = s.replace('.',',');
                }
                text = list.join('\t');
            }
        }
        if (list.count() == 13) {
            if (list.at(0) == "Beleg") {
                for (int i = 6;i < 12; i++){
                    QString s = list.at(i);
                    list[i] = s.replace(',','.');
                    qsum += s;
                    list[i] = s.replace('.',',');
                }
            }
            sum += QBCMath::bcround(qsum.toString(), 2);
            sum.round(2);
            list[11] = sum.toString().replace('.',',');
            text = list.join('\t');
        }

        SecureByteArray sa = text.toUtf8();
        text = Crypto::encrypt(sa, SecureByteArray("Journal"));
        QString checksum = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha1).toHex().toUpper();

        query2.bindValue(":id", query.value("id").toInt());
        query2.bindValue(":version",query.value("version").toString());
        query2.bindValue(":cashregisterid", query.value("cashregisterid").toString());
        query2.bindValue(":datetime", query.value("datetime").toString());
        query2.bindValue(":data", text);
        query2.bindValue(":checksum", checksum);
        query2.bindValue(":userid", query.value("userid").toInt());
        query2.exec();
    }

    query.exec("SELECT checksum FROM journal");
    if (query.next())
        query.exec("DROP TABLE journal_backup");

    if (!dbc.commit())
        dbc.rollback();

}
