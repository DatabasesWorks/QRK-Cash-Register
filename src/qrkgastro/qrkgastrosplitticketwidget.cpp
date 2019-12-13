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

#include "qrkgastro.h"
#include "qrkgastrosplitticketwidget.h"
#include "database.h"
#include "history.h"
#include "ui_qrkgastrosplitticketwidget.h"

#include <QSqlRelationalTableModel>
#include <QSqlQuery>

QRKGastroSplitTicketWidget::QRKGastroSplitTicketWidget(bool moveTable, QWidget *parent):
    QDialog(parent), ui(new Ui::QRKGastroSplitTicketWidget), m_movetable(moveTable)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->tableSelectFrame->setVisible(m_movetable);

    connect(ui->oldOrderList, &QTreeWidget::itemClicked, this, &QRKGastroSplitTicketWidget::toNew);
    connect(ui->newOrderList, &QTreeWidget::itemClicked, this, &QRKGastroSplitTicketWidget::fromNew);
    connect(ui->doneButton, &QPushButton::clicked, this, static_cast<void(QRKGastroSplitTicketWidget::*)()>(&QRKGastroSplitTicketWidget::done));
    connect(ui->cancelButton, &QPushButton::clicked, this, &QRKGastroSplitTicketWidget::cancel);
    if (m_movetable)
        connect(ui->roomComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &QRKGastroSplitTicketWidget::tableData);

    ui->oldOrderList->header()->resizeSection(0, 45);
    ui->newOrderList->header()->resizeSection(0, 45);

    if (m_movetable) {
        QSqlDatabase dbc = Database::database();
        m_tableModel = new QSqlRelationalTableModel(this, dbc);
        ui->tableComboBox->setModel(m_tableModel);
        ui->tableComboBox->setModelColumn(1);  // show name

        m_roomModel = new QSqlRelationalTableModel(this, dbc);
        m_roomModel->setQuery("SELECT id, name FROM rooms", dbc);
        ui->roomComboBox->setModel(m_roomModel);
        ui->roomComboBox->setModelColumn(1);  // show name
    }

}

QRKGastroSplitTicketWidget::ReturnValue QRKGastroSplitTicketWidget::exec(int &ticket, int table)
{
    ui->tableLabel->setText(QRKGastro::getTableName(table));
    ui->newOrderList->clear();

    QRKGastro::fillOrderList(ui->oldOrderList, ticket);

    ReturnValue ret = static_cast<QRKGastroSplitTicketWidget::ReturnValue>(eventLoop.exec());

    if ( ret == CANCELED )
        return CANCELED;

    // do the split
    // clean up the new list: discard old order_id, remove hidden items
    for (int i = 0; i < ui->newOrderList->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = ui->newOrderList->topLevelItem(i);
        if ( item->isHidden() ) {
            delete item;
            i--;
            continue;
        }
        item->setData(1, QRKGastro::ORDER_ID, -1);
    }

    int ticketId = 0;
    if (m_movetable) {
        int newtable = getNewTableId(table);
        if ( !QRKGastro::createOrUpdateTicket(ui->newOrderList, ticketId, newtable, false) )
            return CANCELED;  // nothing in the new list - bail out
    } else {
        if ( !QRKGastro::createOrUpdateTicket(ui->newOrderList, ticketId, table, false) )
            return CANCELED;  // nothing in the new list - bail out
    }

    QRKGastro::createOrUpdateTicket(ui->oldOrderList, ticket, table);

    ticket = ticketId;  // really created a new ticket; return new id

    return SPLITTED;
}

void QRKGastroSplitTicketWidget::tableData(int id)
{
    QSqlDatabase dbc = Database::database();
    int roomId = m_roomModel->data(m_roomModel->index(id, 0), Qt::DisplayRole).toInt();
    QSqlQuery query(dbc);
    query.prepare("SELECT id, name FROM tables WHERE roomId=:roomId");
    query.bindValue(":roomId", roomId);
    query.exec();
    m_tableModel->setQuery(query);
    ui->tableComboBox->setModelColumn(1);  // show name
}

int QRKGastroSplitTicketWidget::getNewTableId(int table)
{
    if (m_movetable) {
        int tableId = m_tableModel->data(m_tableModel->index(ui->tableComboBox->currentIndex(), 0), Qt::DisplayRole).toInt();
        if (tableId == 0)
            return table;
        return tableId;
    }
    return table;
}

void QRKGastroSplitTicketWidget::done()
{
    if(m_movetable)
        eventLoop.exit(MOVED);
    else
        eventLoop.exit(SPLITTED);
}

void QRKGastroSplitTicketWidget::cancel()
{
    eventLoop.exit(CANCELED);
}

// find given order in given tree

QTreeWidgetItem *QRKGastroSplitTicketWidget::findSameOrder(QTreeWidget *tree, const QTreeWidgetItem *order)
{
    for (int i = 0; i < tree->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = tree->topLevelItem(i);
        if ( item->data(1, QRKGastro::ORDER_ID) == order->data(1, QRKGastro::ORDER_ID) )
            return item;
    }

    return Q_NULLPTR;
}

void QRKGastroSplitTicketWidget::moveItem(QTreeWidgetItem *selected, QTreeWidget *target)
{
    if ( !selected || selected->parent() )  // do nothing on child items (extras)
        return;

    // reduce/remove in old list
    selected->setData(0, Qt::DisplayRole, selected->data(0, Qt::DisplayRole).toInt() - 1);

    if ( selected->data(0, Qt::DisplayRole).toInt() == 0 )
        selected->setHidden(true);

    // increase/add in new list
    // find order in target tree and update count, or add as new item
    QTreeWidgetItem *item = findSameOrder(target, selected);

    if ( item ) {
        item->setData(0, Qt::DisplayRole, item->data(0, Qt::DisplayRole).toInt() + 1);    // count
        item->setHidden(false);
    } else { // not in list, add as new item
        QTreeWidgetItem *item = new QTreeWidgetItem;

        item->setData(0, Qt::DisplayRole, 1);    // count
        item->setData(1, Qt::DisplayRole, selected->data(1, Qt::DisplayRole)); // product name
        item->setData(1, QRKGastro::PRODUCT_ID, selected->data(1, QRKGastro::PRODUCT_ID));
        item->setData(1, QRKGastro::PRODUCT_PRICE, selected->data(1, QRKGastro::PRODUCT_PRICE));
        item->setData(1, QRKGastro::ORDER_ID, selected->data(1, QRKGastro::ORDER_ID));
        item->setSizeHint(0, QSize(50, 50));
        target->addTopLevelItem(item);

        // add also all extras
        for (int j = 0; j < selected->childCount(); j++) {
            QTreeWidgetItem *child = new QTreeWidgetItem(item);
            child->setData(0, Qt::DisplayRole, selected->child(j)->data(0, Qt::DisplayRole));
            child->setData(1, QRKGastro::EXTRA_TYPE, selected->child(j)->data(1, QRKGastro::EXTRA_TYPE));
            child->setData(1, Qt::DisplayRole, selected->child(j)->data(1, Qt::DisplayRole));
            child->setData(1, QRKGastro::PRODUCT_ID, selected->child(j)->data(1, QRKGastro::PRODUCT_ID));

            item->setExpanded(true);
        }

        target->scrollToBottom();
        target->setCurrentItem(item);
    }
}

void QRKGastroSplitTicketWidget::toNew(QTreeWidgetItem *selected, int column)
{
    Q_UNUSED(column)
    moveItem(selected, ui->newOrderList);
}

void QRKGastroSplitTicketWidget::fromNew(QTreeWidgetItem *selected, int column)
{
    Q_UNUSED(column)
    moveItem(selected, ui->oldOrderList);
}
