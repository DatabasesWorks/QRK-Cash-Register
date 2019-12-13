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

#ifndef QRKGASTROTABLEMANAGER_H
#define QRKGASTROTABLEMANAGER_H

#include <QDialog>

class QSqlQueryModel;

namespace Ui {
class QRKGastroTableManager;
}

class QRKGastroTableManager : public QDialog
{
    Q_OBJECT

public:
    explicit QRKGastroTableManager(QWidget *parent = Q_NULLPTR);
    ~QRKGastroTableManager();

private:
    Ui::QRKGastroTableManager *ui;

    void roomClicked(QModelIndex idx);
    void tableClicked(QModelIndex idx);

    int getRoomId(const QString &name);
    QString getRoomName(int id);
    int getTableId(const QString &name);
    QString getTableName(int id);

    void fillTableView(const QString &name);

    void newRoom();
    void editRoom();
    void deleteRoom();
    void refreshRooms();

    void newTable();
    void editTable();
    void deleteTable();
    bool hasTableOpenTickets(int tableId);
    QStringList getAllOpenTablesList(int roomId);

    QSqlQueryModel *m_roomsModel;
    QSqlQueryModel *m_tableModel;
    int m_currentRoomId = 0;
    int m_currentTableId = 0;
};

#endif // QRKGASTROTABLEMANAGER_H
