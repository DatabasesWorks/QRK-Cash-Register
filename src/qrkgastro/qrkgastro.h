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

#ifndef QRKGASTRO_H
#define QRKGASTRO_H

#include <QWidget>

class QRKGastroSelector;
class QRKGastroOpenTickets;
class QRKGastroTableOrder;
class QrkGastroCurfewChecker;
class QTreeWidget;

namespace Ui {
class QRKGastro;
}

class QRKGastro : public QWidget
{
    Q_OBJECT

public:
    // used in orderList (fillOrderList)
    enum COLUMNS
    {
        PRODUCT_ID    = Qt::UserRole,
        PRODUCT_PRICE = Qt::UserRole + 1,
        EXTRA_TYPE    = Qt::UserRole + 2,    // orderExtras.type: 1=with, 0=without
        ORDER_ID      = Qt::UserRole + 3,     // orders.id for the extra items
        ORDER_DESCRIPTION      = Qt::UserRole + 4     // orders.id for the extra items
    };
    // orderExtras.type
    enum
    {
        TYPE_WITH = 1,
        TYPE_WITHOUT = 0
    };

    // cancellation
    enum
    {
        CANCEL_FALSETALLIED = 1,
        CANCEL_WAITTOLONG,
        CANCEL_WARESPOILED,
        CANCEL_CANCELLATION,
        CANCEL_OTHERGUESTWISH
    };


public:
    explicit QRKGastro(QWidget *parent = Q_NULLPTR);
    ~QRKGastro();
    void init();
    static void fillOrderList(QTreeWidget *tree, int ticket);
    static int getFirstRoomId();
    static QString getTableName(int num);
    static QString getRoomName(int roomId);
    static QString getRoomNameFromTableId(int tableId);
    static QString getOrderSum(int table);
    static bool isOrderNotServed(int tableId);

    static bool createOrUpdateTicket(QTreeWidget *tree, int &ticket, int table, bool asServed = false);

signals:
    void cancelGastroButton_clicked(bool clicked = true);

private:
    Ui::QRKGastro *ui;

    void curfewdiff(int diff);
    void tableOrder(int id);
    void newTableOrder(int id);
    void changeTableOrderTicket(int tableId, int ticketId);
    void updateButton(int id);

    void cancelTableOrder(int tableId, bool leaveTicket);
    void leaveOpenTickets();
    static void updateOrderDescription(int id, const QString &descripton);
    int getRoomIdFromTableId(int tableId);
    bool openTickets();

    QRKGastroSelector *m_selector = Q_NULLPTR;
    QRKGastroOpenTickets *m_openTickets = Q_NULLPTR;
    QRKGastroTableOrder *m_order = Q_NULLPTR;
    QrkGastroCurfewChecker *m_checker = Q_NULLPTR;
    QThread *m_checkerThread = Q_NULLPTR;
    int m_lastTableId = 0;
};

#endif // QRKGASTRO_H
