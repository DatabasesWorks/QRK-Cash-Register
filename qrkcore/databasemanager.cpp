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

#include "databasemanager.h"

#include <QSqlDatabase>
#include <QMutexLocker>
#include <QThread>
#include <QSqlError>
#include <QDebug>

QMutex DatabaseManager::s_databaseMutex;
QHash<QThread*, QHash<QString, QSqlDatabase> > DatabaseManager::s_instances;

QSqlDatabase DatabaseManager::database(const QString& connectionName)
{
    QMutexLocker locker(&s_databaseMutex);
    QThread *thread = QThread::currentThread();

    // if we have a connection for this thread, return it
    QHash<QThread*, QHash<QString, QSqlDatabase> >::Iterator it_thread = s_instances.find(thread);
    if (it_thread != s_instances.end()) {
        QHash<QString, QSqlDatabase>::iterator it_conn = it_thread.value().find(connectionName);
        if (it_conn != it_thread.value().end()) {
            QSqlDatabase connection = it_conn.value();
//            qDebug() << "Function Name: " << Q_FUNC_INFO << " found SQL connection instances Thread: " << thread->currentThreadId() << " Name: " << connectionName;
            if (connection.isValid())
                return it_conn.value();
        }
    }

    // otherwise, create a new connection for this thread
    QSqlDatabase connection = QSqlDatabase::cloneDatabase(
                                  QSqlDatabase::database(connectionName),
                                  QString("%1_%2").arg(connectionName).arg((long)thread));

    // open the database connection
    // initialize the database connection
    if (!connection.open()) {
        // Todo: Exeption Handling
        qCritical() << "Function Name: " << Q_FUNC_INFO << connection.lastError().text();
        return connection;
    }

    qDebug() << "Function Name: " << Q_FUNC_INFO << " new SQL connection instances Thread: " << thread->currentThreadId() << " Name: " << connectionName;

    s_instances[thread][connectionName] = connection;
    return connection;
}
