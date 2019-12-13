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
 * LillePOS parts Copyright 2010, Martin Koller, kollix@aon.at
 */

#ifndef QRKGASTROTABLEORDER_H
#define QRKGASTROTABLEORDER_H

#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class History;

namespace Ui {
class QRKGastroTableOrder;
}

class QRKGastroTableOrder : public QWidget
{
    Q_OBJECT

public:
    explicit QRKGastroTableOrder(QWidget *parent = Q_NULLPTR);
    ~QRKGastroTableOrder();
    void setTicketId(int id);
    void setTableId(int id);
    void refresh();

signals:
    void cancelOrder(int tableId, bool tableorder = false);
    void updateOrderButton(int tableId);
    void payTicket(int ticketId);

private:
    Ui::QRKGastroTableOrder *ui;
    History *m_history;
    void addSelectedProduct(int product);
    void itemClicked(QTreeWidgetItem *item, int column);

//    void changeTicket(int tableId, int ticketId);
    void fillOrderList(QTreeWidget *tree, int ticket);
    void updateOrderSum();
    void doneSlot(bool printOrders);
    void printSlot();
    void printUnprintedOrders(int table);
    void payNowSlot();
    void cancelSlot();
    bool finishOrder();
    void plusSlot();
    void minusSlot();
    void removeSlot();

    void withButtonSlot();
    void withoutButtonSlot();
    void quickProduct();
    int getCountOfProduct(const QList<QTreeWidgetItem*> &selected);

    void readSettings();
    void writeSettings();

    int m_currentTable, m_currentTicket;
};

#endif // QRKGASTROTABLEORDER_H
