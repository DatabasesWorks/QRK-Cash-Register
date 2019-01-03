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

#include "demomode.h"
#include "database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

bool DemoMode::isDemoMode()
{
  QSqlDatabase dbc = Database::database();
  QSqlQuery query(dbc);
  query.prepare("SELECT value FROM globals WHERE name='demomode'");
  query.exec();
  if (query.next())
    return query.value("value").toBool();

  return !isModeNotSet();

}

bool DemoMode::isModeNotSet()
{
  QSqlDatabase dbc = Database::database();

  QSqlQuery query(dbc);
  bool ok = query.prepare("SELECT value FROM globals WHERE name='demomode'");
  ok = query.exec();
  if (!ok) {
      qWarning() << "Function Name: " << Q_FUNC_INFO << " " << query.lastError().text();
      qWarning() << "Function Name: " << Q_FUNC_INFO << " " << Database::getLastExecutedQuery(query);
      qWarning() << "Function Name: " << Q_FUNC_INFO << " " << dbc.lastError().text();
  }
  if (query.next())
    return false;

  query.prepare("INSERT INTO globals (name, value) VALUES(:mode, :value)");
  query.bindValue(":mode", "demomode");
  query.bindValue(":value", true);

  query.exec();

  return true;

}

void DemoMode::leaveDemoMode()
{
  QSqlDatabase dbc = Database::database();
  QSqlQuery query(dbc);
  query.prepare("UPDATE globals SET value=:value WHERE name='demomode'");
  query.bindValue(":value", false);
  query.exec();

}
