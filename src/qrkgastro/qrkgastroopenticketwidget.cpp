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
#include "qrkgastroopenticketwidget.h"
#include "qrkgastrotableorder.h"
#include "database.h"
#include "3rdparty/qbcmath/bcmath.h"

#include <QTreeWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QEvent>
#include <QPushButton>

TreeWidget::TreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setTextElideMode(Qt::ElideMiddle);
    setExpandsOnDoubleClick(false);
}

bool TreeWidget::viewportEvent(QEvent *event)
{
    if ( event->type() == QEvent::MouseButtonPress )
        emit selected();

    return QTreeWidget::viewportEvent(event);
}

QrkGastroOpenTicketWidget::QrkGastroOpenTicketWidget(QWidget *parent, int theId, int theTable)
    : QFrame(parent), id(theId), table(theTable), selected(false), toServeWidget(Q_NULLPTR)
{
    setAutoFillBackground(true);
    setFixedWidth(300);
    setFrameShape(QFrame::Box);
    setSelected(selected);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout;
    QHBoxLayout *hbox2 = new QHBoxLayout;

    QLabel *tableLabel = new QLabel(this);
    tableLabel->setAlignment(Qt::AlignRight);

    tableLabel->setText(tr("Tisch: <b>%1/%2</b>").arg(QRKGastro::getRoomNameFromTableId(table)).arg(QRKGastro::getTableName(table)));
    hbox->addWidget(tableLabel);

    TreeWidget *ticketTree = new TreeWidget(this);
    QTreeWidgetItem *headerItem = ticketTree->headerItem();
    headerItem->setText(0, tr("Anz"));
    headerItem->setText(1, tr("Produkt"));
    headerItem->setText(2, tr("je"));  // price per piece

//    ticketTree->header()->setStretchLastSection(false);
    //ticketTree->header()->setResizeMode(0, QHeaderView::ResizeToContents);
//    ticketTree->header()->resizeSection(0, 33);
    ticketTree->setRootIsDecorated(false);
    ticketTree->setSelectionMode(QAbstractItemView::NoSelection);

    connect(ticketTree, &TreeWidget::selected, this, &QrkGastroOpenTicketWidget::selectedSlot);
//    connect(ticketTree, &TreeWidget::selected, this, &QrkGastroOpenTicketWidget::selectionChanged);

    QSqlDatabase dbc = Database::database();
    QSqlQuery orders(dbc);

    orders.prepare("SELECT ticketorders.count, products.name, ticketorders.gross, ticketorders.id FROM ticketorders "
                   " LEFT JOIN products ON ticketorders.product=products.id"
                   " WHERE ticketorders.ticketId=:id");
    orders.bindValue(":id", id);

    bool ok = orders.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << orders.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(orders);
    }
    QBCMath sum(0);
    while ( orders.next() ) {

        QBCMath gross(orders.value(2).toDouble());
        gross.round(2);
        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setData(0, Qt::DisplayRole, orders.value("count").toInt());    // count
        item->setData(1, Qt::DisplayRole, orders.value("name").toString()); // product name
        item->setData(2, Qt::DisplayRole, gross.toLocale()); // product price
        item->setTextAlignment(2, Qt::AlignRight);
        ticketTree->addTopLevelItem(item);

        sum += (gross * orders.value("count").toInt());

        // check for orderExtras
        QSqlQuery extras(dbc);
        extras.prepare("SELECT orderExtras.type, products.name FROM orderExtras "
                       " LEFT JOIN products ON orderExtras.product=products.id"
                       " WHERE orderId=:id");
        extras.bindValue(":id",orders.value("id").toInt());

        ok = extras.exec();
        if (!ok) {
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << extras.lastError().text();
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(extras);
        }
        while ( extras.next() )
        {
            QTreeWidgetItem *child = new QTreeWidgetItem(item);
            child->setData(0, Qt::DisplayRole, (extras.value("type").toInt() == QRKGastro::TYPE_WITH) ? "+" : "-");
            child->setData(1, Qt::DisplayRole, extras.value("name").toString());

            item->setExpanded(true);
        }
    }

    sum.round(2);
    QLabel *sumLabel = new QLabel(QString("Summe: %1").arg(sum.toLocale()), this);
    sumLabel->setAlignment(Qt::AlignRight);
    hbox2->addWidget(sumLabel);
    vbox->addLayout(hbox);
    vbox->addLayout(hbox2);
    vbox->addWidget(ticketTree);
    resizeColumnsToContents(*ticketTree);
    // show unprinted orders
    orders.prepare("SELECT (ticketorders.count - ticketorders.printed) AS count, products.name, ticketorders.id FROM ticketorders "
                   " LEFT JOIN products ON ticketorders.product=products.id"
                   " WHERE ticketorders.ticketId=:id AND (ticketorders.count > ticketorders.printed)");
    orders.bindValue(":id", id);
    ok = orders.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << orders.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(orders);
    }

    if ( orders.next() ) {
        toServeWidget = new QWidget(this);
        QVBoxLayout *vb = new QVBoxLayout(toServeWidget);
        vb->setContentsMargins(0, 0, 0, 0);
        QHBoxLayout *hb = new QHBoxLayout;
        hb->addWidget(new QLabel(tr("Zu Servieren"), this));
        QPushButton *servedButton = new QPushButton(tr("Serviert"));
        hb->addWidget(servedButton);
        connect(servedButton, SIGNAL(clicked()), this, SLOT(ordersServed()));
        vb->addLayout(hb);

        QTreeWidget *toServe = new TreeWidget(this);
        QTreeWidgetItem *headerItem = toServe->headerItem();
        headerItem->setText(0, tr("Anz"));
        headerItem->setText(1, tr("Produkt"));

        toServe->setColumnCount(2);
//        toServe->header()->resizeSection(0, 40);
        toServe->setRootIsDecorated(false);
        toServe->setSelectionMode(QAbstractItemView::NoSelection);

        do {
            QTreeWidgetItem *item = new QTreeWidgetItem;
            item->setData(0, Qt::DisplayRole, orders.value("count").toInt());    // count
            item->setData(1, Qt::DisplayRole, orders.value("name").toString()); // product name
            toServe->addTopLevelItem(item);

            // check for orderExtras
            QSqlQuery extras(dbc);
            extras.prepare("SELECT orderExtras.type, products.name FROM orderExtras "
                           " LEFT JOIN products ON orderExtras.product=products.id"
                           " WHERE orderId=:id");
            extras.bindValue(":id", orders.value("id").toInt());
            ok = extras.exec();
            if (!ok) {
                qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << extras.lastError().text();
                qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(extras);
            }

            while ( extras.next() ) {
                QTreeWidgetItem *child = new QTreeWidgetItem(item);
                child->setData(0, Qt::DisplayRole, (extras.value("type").toInt() == QRKGastro::TYPE_WITH) ? "+" : "-");
                child->setData(1, Qt::DisplayRole, extras.value("name").toString());

                item->setExpanded(true);
            }
        }
        while ( orders.next() );

        vb->addWidget(toServe);

        vbox->addWidget(toServeWidget);
        resizeColumnsToContents(*toServe);

        connect(toServe, SIGNAL(selected()), this, SLOT(selectedSlot()));
    }
}

void QrkGastroOpenTicketWidget::setSelected(bool sel)
{
    selected = sel;

    setBackgroundRole(selected ? QPalette::Highlight : QPalette::AlternateBase);
}

void QrkGastroOpenTicketWidget::selectedSlot()
{
    setSelected(!selected);
    emit selectionChanged();
}

// mark unserverd orders as served

void QrkGastroOpenTicketWidget::ordersServed()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery orders(dbc);

    orders.prepare("UPDATE ticketorders SET printed=count WHERE ticketId=:ticketId");
    orders.bindValue(":ticketId", id);
    bool ok = orders.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << orders.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(orders);
    }
    toServeWidget->hide();
}

void QrkGastroOpenTicketWidget::resizeColumnsToContents(QTreeWidget &treeWidget)
{
  int cCols = treeWidget.columnCount();
  int cItems = treeWidget.topLevelItemCount();
  int w;
  int col;
  int i;
  for( col = 0; col < cCols; col++ ) {
    w = treeWidget.header()->sectionSizeHint( col );
    for( i = 0; i < cItems; i++ )
      w = qMax( w, treeWidget.topLevelItem( i )->text( col ).size()*7 + (col == 0 ? treeWidget.indentation() : 0) );
    treeWidget.header()->resizeSection( col, w );
  }
}
