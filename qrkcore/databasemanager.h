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

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include "qrkcore_global.h"

#include <QMutex>
#include <QHash>
#include <QSqlDatabase>
#include <QMap>

class QThread;

class QRK_EXPORT DatabaseManager
{
    public:
        static QSqlDatabase database(const QString& connectionName = QLatin1String(QSqlDatabase::defaultConnection));
        static void clear();
        static void removeCurrentThread(QString);

    private:
        static QMutex s_databaseMutex;
        static QMap<QString, QMap<QString, QSqlDatabase>> s_instances;

};

#endif // DATABASEMANAGER_H
