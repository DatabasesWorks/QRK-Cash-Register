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

#ifndef QRKGASTROOPENTICKETS_H
#define QRKGASTROOPENTICKETS_H

#include <QWidget>

class QTreeWidget;

namespace Ui {
class QRKGastroOpenTickets;
}

class QRKGastroOpenTickets : public QWidget
{
    Q_OBJECT

public:
    explicit QRKGastroOpenTickets(QWidget *parent = Q_NULLPTR);
    ~QRKGastroOpenTickets();
    int setTableId(int id);
    void payTicket(int tickeId);

signals:
    void newTicket(int tableid);
    void changeTicket(int tableId, int ticketId);
    void leaveTicket();

private:
    Ui::QRKGastroOpenTickets *ui;
    void selectionChanged();
    void newTicket(bool clicked);
    void changeTicket(int ticket);
    void voidTicket();
    void changeTicket(bool clicked);
    void getSelectedTicket(int &id, int &table);
    void moveTicket();
    void splitTicket(bool move =false);
    bool finishTicket(int ticket);
    void payTicket();
    void payGroupTicket(QList<int> tickets);
    void refresh();
    void infoMessage();

    int m_currentTable;
};

#endif // QRKGASTROOPENTICKETS_H
