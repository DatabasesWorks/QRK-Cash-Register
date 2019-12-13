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

#include "qrkgastroopenticketslistwidget.h"
#include "qrkgastroopenticketwidget.h"
#include "database.h"

#include <QHBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

QrkGastroOpenTicketsListWidget::QrkGastroOpenTicketsListWidget(QWidget *parent)
    : QScrollArea(parent)
{
    setWidgetResizable(true);

    QWidget *contentsWidget = new QWidget(this);
//    contentsWidget->resize(1348, 682);
    new QHBoxLayout(contentsWidget);

    setWidget(contentsWidget);
}

void QrkGastroOpenTicketsListWidget::refreshTickets(int tableid)
{
    // clear previous contents
    QLayoutItem *item;
    while ( (item = widget()->layout()->takeAt(0)) ) {
        delete item->widget();
        delete item;
    }
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT `tickets`.`id`, `tickets`.`tableId` FROM tickets"
                  " LEFT JOIN `tables` ON `tickets`.`tableId`=`tables`.`id`"
                  " WHERE open=1 AND `tickets`.`tableId`=:tableid ORDER BY `tables`.`name`");
    query.bindValue(":tableid", tableid);
    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
    while ( query.next() ) {
        QrkGastroOpenTicketWidget *ticketTree = new QrkGastroOpenTicketWidget(this, query.value("id").toInt(), query.value("tableId").toInt());
        widget()->layout()->addWidget(ticketTree);
        connect(ticketTree, &QrkGastroOpenTicketWidget::selectionChanged, this, &QrkGastroOpenTicketsListWidget::selectionChanged);
    }

    static_cast<QHBoxLayout*>(widget()->layout())->addStretch();
}

void QrkGastroOpenTicketsListWidget::selectTicket(int id)
{
    for (int i = 0; i < widget()->layout()->count(); i++) {
        QLayoutItem *item = widget()->layout()->itemAt(i);
        if ( item->widget() && (static_cast<QrkGastroOpenTicketWidget*>(item->widget())->getId() == id) ) {
            static_cast<QrkGastroOpenTicketWidget*>(item->widget())->setSelected(true);
            break;
        }
    }
}

QList<int> QrkGastroOpenTicketsListWidget::getSelectedTickets() const
{
    QList<int> list;

    for (int i = 0; i < widget()->layout()->count(); i++) {
        QLayoutItem *item = widget()->layout()->itemAt(i);
        if ( item->widget() && static_cast<QrkGastroOpenTicketWidget*>(item->widget())->isSelected() )
            list.append(static_cast<QrkGastroOpenTicketWidget*>(item->widget())->getId());
    }

    return list;
}

QList<int> QrkGastroOpenTicketsListWidget::getTickets() const
{
    QList<int> list;

    for (int i = 0; i < widget()->layout()->count(); i++) {
        QLayoutItem *item = widget()->layout()->itemAt(i);
        if ( item->widget() )
            list.append(static_cast<QrkGastroOpenTicketWidget*>(item->widget())->getId());
    }

    return list;
}

int QrkGastroOpenTicketsListWidget::getTableOfTicket(int id)
{
    for (int i = 0; i < widget()->layout()->count(); i++) {
        QLayoutItem *item = widget()->layout()->itemAt(i);
        if ( item->widget() && (static_cast<QrkGastroOpenTicketWidget*>(item->widget())->getId() == id) ) {
            return static_cast<QrkGastroOpenTicketWidget*>(item->widget())->getTable();
        }
    }

    return 0;
}
