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

#include "database.h"
#include "databasemanager.h"

#include <QSqlDatabase>
#include <QMutexLocker>
#include <QThread>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QDebug>

QMutex DatabaseManager::s_databaseMutex;
QMap<QString, QMap<QString, QSqlDatabase> > DatabaseManager::s_instances;

QSqlDatabase DatabaseManager::database(const QString& connectionName)
{
    QMutexLocker locker(&s_databaseMutex);
    QThread *thread = QThread::currentThread();

    if (!thread->objectName().isEmpty()) {
        QString objectname = QString("%1_%2").arg(connectionName).arg(QString::number(qint64(QThread::currentThread()), 16));

        // if we have a connection for this thread, return it
        QMap<QString, QMap<QString, QSqlDatabase> >::Iterator it_thread = s_instances.find(objectname);
        if (it_thread != s_instances.end()) {
            QMap<QString, QSqlDatabase>::iterator it_conn = it_thread.value().find(objectname);
            if (it_conn != it_thread.value().end()) {
                QSqlDatabase connection = it_conn.value();
                // qDebug() << "Function Name: " << Q_FUNC_INFO << " found SQL connection instances Thread: " << thread << " Name: " << connectionName;
                if (connection.isValid()) {
                    return it_conn.value();
                }
            }
        }
    }

    QString objectname = QString("%1_%2").arg(connectionName).arg(QString::number(qint64(QThread::currentThread()), 16));

    thread->setObjectName(objectname);
    // otherwise, create a new connection for this thread

    // cloneDatabase will not work with QT 5.11
    /*    QSqlDatabase connection = QSqlDatabase::cloneDatabase(
                                  QSqlDatabase::database(connectionName),
                                  QString("%1_%2").arg(connectionName).arg(objectname));
    */

    QJsonObject connectionDefinition = Database::getConnectionDefinition();
    QString dbtype = connectionDefinition.value("dbtype").toString();
    QSqlDatabase connection = QSqlDatabase::addDatabase(dbtype, objectname);

    if (dbtype == "QMYSQL") {
        connection.setHostName(connectionDefinition.value("databasehost").toString());
        connection.setUserName(connectionDefinition.value("databaseusername").toString());
        connection.setPassword(connectionDefinition.value("databasepassword").toString());
        connection.setConnectOptions(connectionDefinition.value("databaseoptions").toString());
    }
    connection.setDatabaseName(connectionDefinition.value("databasename").toString());

    // open the database connection
    // initialize the database connection
    if (!connection.open()) {
        // Todo: Exeption Handling
        qCritical() << "Function Name: " << Q_FUNC_INFO << connection.lastError().text();
        return connection;
    }

    qDebug() << "Function Name: " << Q_FUNC_INFO << " new SQL connection instances Thread: " << thread->currentThread() << " Name: " << connectionName;

    enableForeignKey(connection);

    s_instances[objectname][connection.connectionName()] = connection;
    qDebug() << "Function Name: " << Q_FUNC_INFO << " connection instances  used: " << s_instances.size();

    return connection;
}

void DatabaseManager::clear()
{
    s_instances.clear();
}

void DatabaseManager::removeCurrentThread(const QString connectionName)
{

    QString objectname = QString("%1_%2").arg(connectionName).arg(QString::number(qint64(QThread::currentThread()), 16));

    if (s_instances.contains(objectname)) {
        QMap<QString, QSqlDatabase> map = s_instances.value(objectname);
        QString currentConnectionName = "";

        {
            QSqlDatabase connection = map.value(objectname);
            currentConnectionName = connection.connectionName();
            connection.close();
            map.remove(currentConnectionName);
            s_instances.remove(objectname);
        }

        QSqlDatabase::removeDatabase(currentConnectionName);
        qDebug() << "Function Name: " << Q_FUNC_INFO << " remove connection instance: " << objectname;
    }

    qDebug() << "Function Name: " << Q_FUNC_INFO << " connection instances  used: " << s_instances.size();
}

void DatabaseManager::enableForeignKey(QSqlDatabase dbc)
{
    if (dbc.driverName() == "QSQLITE") {
        QSqlQuery query(dbc);
        // enforce foreign key constraint
        query.exec("PRAGMA foreign_keys = 1;");
    }
}
