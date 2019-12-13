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

#include "qrkgastrotablemanager.h"
#include "qrkgastromanagerroomedit.h"
#include "qrkgastromanagertableedit.h"
#include "database.h"
#include "ui_qrkgastrotablemanager.h"

#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QMessageBox>

QRKGastroTableManager::QRKGastroTableManager(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QRKGastroTableManager)
{
    ui->setupUi(this);

    connect(ui->roomsView, &QListView::clicked, this, &QRKGastroTableManager::roomClicked);
    connect(ui->tablesView, &QListView::clicked, this, &QRKGastroTableManager::tableClicked);
    connect(ui->newRoomButton, &QPushButton::clicked, this, &QRKGastroTableManager::newRoom);
    connect(ui->changeRoomButton, &QPushButton::clicked, this, &QRKGastroTableManager::editRoom);
    connect(ui->deleteRoomButton, &QPushButton::clicked, this, &QRKGastroTableManager::deleteRoom);
    connect(ui->newTableButton, &QPushButton::clicked, this, &QRKGastroTableManager::newTable);
    connect(ui->changeTableButton, &QPushButton::clicked, this, &QRKGastroTableManager::editTable);
    connect(ui->deleteTableButton, &QPushButton::clicked, this, &QRKGastroTableManager::deleteTable);

    connect(ui->exitButton, &QPushButton::clicked, this, &QRKGastroTableManager::accept);

    QSqlDatabase dbc = Database::database();
    m_roomsModel = new QSqlQueryModel();
    m_roomsModel->setQuery("SELECT name FROM rooms", dbc);
    m_tableModel = new QSqlQueryModel();

    ui->roomsView->setModel(m_roomsModel);
    ui->tablesView->setModel(m_tableModel);
    ui->roomsView->selectionModel()->select( m_roomsModel->index(0,0), QItemSelectionModel::Select );
    roomClicked(m_roomsModel->index(0, 0));
}

QRKGastroTableManager::~QRKGastroTableManager()
{
    delete ui;
}

void QRKGastroTableManager::roomClicked(QModelIndex idx)
{
    QString roomName = idx.data().toString();
    m_currentRoomId = getRoomId(roomName);
    m_currentTableId = 0;
    fillTableView(roomName);
    ui->deleteRoomButton->setEnabled(m_currentRoomId != 0);
    ui->changeRoomButton->setEnabled(m_currentRoomId != 0);
    ui->deleteTableButton->setEnabled(false);
    ui->changeTableButton->setEnabled(false);
}

void QRKGastroTableManager::tableClicked(QModelIndex idx)
{
    QString tableName = idx.data().toString();
    m_currentTableId = getTableId(tableName);
    ui->deleteTableButton->setEnabled(m_currentTableId != 0);
    ui->changeTableButton->setEnabled(m_currentTableId != 0);
}

void QRKGastroTableManager::fillTableView(const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT name FROM tables WHERE roomId=:roomId");
    int id = getRoomId(name);
    query.bindValue(":roomId", id);
    query.exec();
    m_tableModel->setQuery(query);
}

int QRKGastroTableManager::getRoomId(const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT id FROM rooms WHERE name=:name");
    query.bindValue(":name", name);
    query.exec();
    if (query.next())
        return query.value("id").toInt();

    return 0;
}

QString QRKGastroTableManager::getRoomName(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT name FROM rooms WHERE id=:id");
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
        return query.value("name").toString();

    return "";
}

int QRKGastroTableManager::getTableId(const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT id FROM tables WHERE name=:name AND roomId=:roomId");
    query.bindValue(":name", name);
    query.bindValue(":roomId", m_currentRoomId);
    query.exec();
    if (query.next())
        return query.value("id").toInt();

    return 0;
}

QString QRKGastroTableManager::getTableName(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT name FROM tables WHERE id=:id");
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
        return query.value("name").toString();

    return "";
}

void QRKGastroTableManager::newRoom()
{
    QRKGastroManagerRoomEdit roomEdit(this);
    roomEdit.exec();
    refreshRooms();
}

void QRKGastroTableManager::editRoom()
{
    QRKGastroManagerRoomEdit roomEdit(this, m_currentRoomId);
    roomEdit.exec();
    refreshRooms();
}

void QRKGastroTableManager::deleteRoom()
{
    if (getAllOpenTablesList(m_currentRoomId).count() > 0) {
        QMessageBox::information(this, tr("Raum löschen"), tr("Aufgrund offener Bonierungen kann der Raum (%1) nicht gelöscht werden.").arg(getRoomName(m_currentRoomId)),
                                       QMessageBox::Yes);
        return;
    }
    if ( QMessageBox::question(this, tr("Raum löschen"), tr("Möchten sie diesen Raum (%1) wirklich löschen ?").arg(getRoomName(m_currentRoomId)),
                               QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("DELETE FROM tables WHERE roomId=:roomId;");
    query.bindValue(":roomId", m_currentRoomId);
    if (query.exec()) {
        QSqlQuery room(dbc);
        room.prepare("DELETE FROM rooms WHERE id=:roomId;");
        room.bindValue(":roomId", m_currentRoomId);
        room.exec();
    }
    refreshRooms();
}

void QRKGastroTableManager::refreshRooms()
{
    QSqlDatabase dbc = Database::database();
    m_roomsModel->setQuery("SELECT name FROM rooms", dbc);
}

void QRKGastroTableManager::newTable()
{
    QRKGastroManagerTableEdit tableEdit(this, m_currentRoomId);
    tableEdit.exec();
    fillTableView(getRoomName(m_currentRoomId));
}

void QRKGastroTableManager::editTable()
{
    QRKGastroManagerTableEdit tableEdit(this, m_currentRoomId, m_currentTableId);
    tableEdit.exec();
    fillTableView(getRoomName(m_currentRoomId));
}

void QRKGastroTableManager::deleteTable()
{
    if (hasTableOpenTickets(m_currentTableId)) {
        QMessageBox::information(this, tr("Tisch löschen"), tr("Aufgrund offener Bonierungen kann der Tisch (%1) nicht gelöscht werden.").arg(getTableName(m_currentTableId)),
                                       QMessageBox::Yes);
        return;
    }
    if ( QMessageBox::question(this, tr("Tisch löschen"), tr("Möchten sie diesen Tisch (Raum %1, Tisch %2) wirklich löschen ?").arg(getRoomName(m_currentRoomId)).arg(getTableName(m_currentTableId)),
                               QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("DELETE FROM tables WHERE id=:tableId;");
    query.bindValue(":tableId", m_currentTableId);
    query.exec();

    fillTableView(getRoomName(m_currentRoomId));
}

QStringList QRKGastroTableManager::getAllOpenTablesList(int roomId)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    QStringList openTablesList;
    query.prepare("SELECT id FROM tables WHERE roomId=:roomId");
    query.bindValue(":roomId", roomId);
    query.exec();
    while (query.next()) {
        QSqlQuery openTables(dbc);
        openTables.prepare("SELECT id FROM tickets WHERE open=1 AND tableId=:tableId");
        openTables.bindValue(":tableId", query.value("id").toInt());
        openTables.exec();
        while (openTables.next()) {
            openTablesList.append(openTables.value("id").toString());
        }
    }
    return openTablesList;
}

bool QRKGastroTableManager::hasTableOpenTickets(int tableId)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery openTables(dbc);
    openTables.prepare("SELECT id FROM tickets WHERE open=1 AND tableId=:tableId");
    openTables.bindValue(":tableId", tableId);
    openTables.exec();
    if (openTables.next()) {
        return true;
    }
    return false;
}
