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

#include "history.h"
#include "database.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "3rdparty/ckvsoft/rbac/crypto.h"
#include "QCryptographicHash"

#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QByteArray>
#include <QDebug>

History::History(QObject *parent)
  : QObject(parent)
{
}

void History::historyInsertLine(QString title,  QString text)
{

  QDateTime dt = QDateTime::currentDateTime();
  QSqlDatabase dbc = Database::database();
  QSqlQuery query(dbc);
  QString val = "(datetime,data,userId)";
  bool ok = query.prepare(QString("INSERT INTO history %1 VALUES(:date, :data, :userId)")
                      .arg(val));

  if (!ok) {
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
  }

  SecureByteArray sa = QString(title + "\t" + text + "\t" + dt.toString(Qt::ISODate)).toUtf8();
  QString data = Crypto::encrypt(sa, SecureByteArray("History"));

  query.bindValue(":date", dt.toString(Qt::ISODate));
  query.bindValue(":data", data);
  query.bindValue(":userId", RBAC::Instance()->getUserId());

  ok = query.exec();

  if (!ok) {
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
    qCritical() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
  }
}
