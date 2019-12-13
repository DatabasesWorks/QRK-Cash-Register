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
 */

#include "documentprinter.h"
#include "3rdparty/ckvsoft/texteditdialog.h"
#include "qrkgastro.h"
#include "qrkgastrotableorder.h"
#include "3rdparty/ckvsoft/numerickeypad.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "history.h"
#include "database.h"
#include "preferences/qrksettings.h"
#include "qrkgastrovoiddialog.h"
#include "qrkprogress.h"
#include "qrkgastroquickproduct.h"
#include "ui_qrkgastrotableorder.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

QRKGastroTableOrder::QRKGastroTableOrder(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QRKGastroTableOrder)
{
    ui->setupUi(this);
    ui->orderList->header()->setStretchLastSection(false);
    ui->orderList->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->orderList->headerItem()->setTextAlignment(2, Qt::AlignCenter);

//    ui->cancelButton->setHidden(true);

    connect(ui->orderList, &QTreeWidget::itemClicked, this, &QRKGastroTableOrder::itemClicked);

    m_history = new History(this);

    QGridLayout *layout = new QGridLayout();
    NumericKeypad *numericKeyPad = new NumericKeypad(false, this);
    layout->addWidget(numericKeyPad,0,0);
    layout->addItem(new QSpacerItem(1,1,QSizePolicy::Expanding,QSizePolicy::Fixed),0,1);
    layout->setColumnStretch(1,1);
    ui->gridLayout->addLayout(layout, ui->gridLayout->rowCount(),0);
    numericKeyPad->setHidden(false);

    connect(ui->quickButtons, &QrkQuickButtons::addProductToOrderList, this, &QRKGastroTableOrder::addSelectedProduct);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QRKGastroTableOrder::cancelSlot);
    connect(ui->doneButton, &QrkPushButton::clicked, this, &QRKGastroTableOrder::doneSlot);
    connect(ui->printButton, &QrkPushButton::clicked, this, &QRKGastroTableOrder::printSlot);
    connect(ui->payNowButton, &QrkPushButton::clicked, this, &QRKGastroTableOrder::payNowSlot);
    connect(numericKeyPad, &NumericKeypad::textChanged, [=]() {
        ui->numPadLabel->setText(numericKeyPad->text());
    });
    connect(ui->withButton, &QPushButton::clicked, this, &QRKGastroTableOrder::withButtonSlot);
    connect(ui->withoutButton, &QPushButton::clicked, this, &QRKGastroTableOrder::withoutButtonSlot);
    connect(ui->plusButton, &QPushButton::clicked, this, &QRKGastroTableOrder::plusSlot);
    connect(ui->minusButton, &QPushButton::clicked, this, &QRKGastroTableOrder::minusSlot);
    connect(ui->removeButton, &QPushButton::clicked, this, &QRKGastroTableOrder::removeSlot);
    connect(ui->quickProductPushButton, &QPushButton::clicked, this, &QRKGastroTableOrder::quickProduct);

    readSettings();
    refresh();
}

QRKGastroTableOrder::~QRKGastroTableOrder()
{
    writeSettings();
    delete ui;
}

void QRKGastroTableOrder::refresh()
{
    emit ui->quickButtons->refresh();

}

void QRKGastroTableOrder::setTicketId(int id)
{
    m_currentTicket = id;
    fillOrderList(ui->orderList, m_currentTicket);
}

void QRKGastroTableOrder::setTableId(int id)
{
    m_currentTable = id;
    ui->infoLabel->setText(QString("%1 / %2").arg(QRKGastro::getRoomNameFromTableId(id)).arg(QRKGastro::getTableName(id)));
}

void QRKGastroTableOrder::addSelectedProduct(int product)
{
    if(product == 0)
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    // if the with/without buttons are checked, we need to add this product as sub-item
    // to the currently selected one
    int extraType = -1;
    if ( ui->withButton->isChecked() )
        extraType = QRKGastro::TYPE_WITH;
    else if ( ui->withoutButton->isChecked() )
        extraType = QRKGastro::TYPE_WITHOUT;

    // make this a one-time action
    ui->withButton->setChecked(false);
    ui->withoutButton->setChecked(false);

    if ( extraType != -1 ) {
        QList<QTreeWidgetItem*> selected = ui->orderList->selectedItems();

        if ( selected.isEmpty() || selected[0]->parent() )  // do not add sub-sub item
            return;

        // find product in children and change type
        bool found = false;
        for (int i = 0; i < selected[0]->childCount(); i++) {
            QTreeWidgetItem *child = selected[0]->child(i);
            if ( child->data(1, QRKGastro::PRODUCT_ID) == product ) {
                child->setData(0, Qt::DisplayRole, (extraType == QRKGastro::TYPE_WITH) ? "+" : "-");
                child->setData(1, QRKGastro::EXTRA_TYPE, extraType);
                child->setHidden(false);

                found = true;
                m_history->historyInsertLine(tr("%1 EXTRA").arg((extraType == QRKGastro::TYPE_WITH) ? "+" : "-"), tr("Artikel %1").arg(Database::getProductNameById(product)));
                break;
            }
        }

        if ( !found ) {
            query.prepare(QString("SELECT name FROM products WHERE id=:id"));
            query.bindValue(":id", product);
            bool ok = query.exec();

            if (!ok) {
                qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
                qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
                return;
            }

            query.next();

            QTreeWidgetItem *child = new QTreeWidgetItem(selected[0]);
            child->setData(0, Qt::DisplayRole, (extraType == QRKGastro::TYPE_WITH) ? "+" : "-");
            child->setData(1, QRKGastro::EXTRA_TYPE, extraType);
            child->setData(1, Qt::DisplayRole, query.value(0).toString());
            child->setData(1, QRKGastro::PRODUCT_ID, product);  // store unique product id
            selected[0]->setExpanded(true);
            m_history->historyInsertLine(tr("%1 EXTRA").arg((extraType == QRKGastro::TYPE_WITH) ? "+" : "-"), tr("Artikel %1").arg(Database::getProductNameById(product)));
        }

        return;
    }

    // find product in tree and update count, but only if this item does not
    // have already extra child-items defined
    bool found = false;
    for (int i = 0; i < ui->orderList->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = ui->orderList->topLevelItem(i);
        if ( (item->childCount() == 0) && (item->data(1, QRKGastro::PRODUCT_ID) == product) ) {
            int numpadvalue = ui->numPadLabel->text().toInt();
            if (numpadvalue == 0)
                numpadvalue = 1;
            if ( item->isHidden() ) {
                item->setData(0, Qt::DisplayRole, numpadvalue);
                item->setHidden(false);
            } else {
                item->setData(0, Qt::DisplayRole, item->data(0, Qt::DisplayRole).toInt() + numpadvalue);
            }

            ui->numPadLabel->clear();
            found = true;
            m_history->historyInsertLine(tr("%1 ARTIKEL").arg((extraType == QRKGastro::TYPE_WITH) ? "+" : "-"), tr("Artikel %1").arg(Database::getProductNameById(product)));
            break;
        }
    }

    if ( !found ) {
        query.prepare(QString("SELECT name, gross FROM products WHERE id=:id"));
        query.bindValue(":id", product);
        bool ok = query.exec();

        if (!ok) {
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
            return;
        }

        query.next();

        int numpadvalue = ui->numPadLabel->text().toInt();
        if (numpadvalue == 0)
            numpadvalue = 1;

        QTreeWidgetItem *item = new QTreeWidgetItem;
        item->setData(0, Qt::DisplayRole, numpadvalue);
        item->setData(1, Qt::DisplayRole, query.value("name").toString());
        item->setData(1, QRKGastro::PRODUCT_ID, product);  // store unique product id
        item->setData(1, QRKGastro::PRODUCT_PRICE, query.value("gross").toDouble());
        item->setData(1, QRKGastro::ORDER_ID, -1);
        item->setIcon(2, QIcon(":src/icons/textfield.png"));
        item->setSizeHint(0, QSize(50, 50));
        ui->orderList->addTopLevelItem(item);
        ui->orderList->scrollToBottom();
        ui->orderList->setCurrentItem(item);
        ui->numPadLabel->clear();
        m_history->historyInsertLine(tr("%1 ARTIKEL").arg((extraType == QRKGastro::TYPE_WITH) ? "+" : "-"), tr("Artikel %1").arg(Database::getProductNameById(product)));
    }

    updateOrderSum();
}

void QRKGastroTableOrder::itemClicked(QTreeWidgetItem *item, int column)
{
    if (item != Q_NULLPTR && item->parent() == Q_NULLPTR  && column == 2) {
        QString data = item->data(column, QRKGastro::ORDER_DESCRIPTION).toString();
        TextEditDialog ted(this);
        ted.setText(data);
        if (ted.exec() == QDialog::Accepted) {
            data = ted.getText();
            item->setData(column, QRKGastro::ORDER_DESCRIPTION, data);
        }
    }
}

/*
void QRKGastroTableOrder::changeTicket(int tableId, int ticketId)
{
    ui->withButton->setChecked(false);
    ui->withoutButton->setChecked(false);
    //      clearGroupsLayout();

    fillOrderList(ui->orderList, m_currentTicket);

    updateOrderSum();
}
*/
void QRKGastroTableOrder::fillOrderList(QTreeWidget *tree, int ticketId)
{
    QRKGastro::fillOrderList(tree, ticketId);
    updateOrderSum();
}

void QRKGastroTableOrder::updateOrderSum()
{
    QBCMath sum(0);

    int itemCount = ui->orderList->topLevelItemCount();
    for (int i = 0; i < itemCount; i++) {
        QTreeWidgetItem *item = ui->orderList->topLevelItem(i);
        if ( !item->isHidden() )  // shall be removed
            sum += item->data(0, Qt::DisplayRole).toInt() * item->data(1, QRKGastro::PRODUCT_PRICE).toDouble();
    }

    sum.round(2);
    ui->sumLabel->setText(sum.toLocale() + " " + QLocale().currencySymbol());
    emit updateOrderButton(m_currentTable);
}

void QRKGastroTableOrder::cancelSlot()
{
    doneSlot(false);
    emit cancelOrder(m_currentTable, true);
}

void QRKGastroTableOrder::payNowSlot()
{
    doneSlot(false);
    emit payTicket(m_currentTicket);
}

void QRKGastroTableOrder::printSlot()
{
    doneSlot(true);
}

void QRKGastroTableOrder::doneSlot(bool printOrders)
{
    if ( !finishOrder() ) {
        if ( printOrders )
            printUnprintedOrders(m_currentTicket);
    }

    emit updateOrderButton(m_currentTable);
    emit cancelOrder(m_currentTable);
}

void QRKGastroTableOrder::printUnprintedOrders(int ticket)
{
    QRKProgress progress;
    progress.setText(tr("Bons werden gedruckt."));
    progress.setWaitMode(true);
    progress.show();
    qApp->processEvents();

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT tableId FROM tickets WHERE id=:id");
    query.bindValue(":id", ticket);
    query.exec();
    query.next();
    int table = query.value("tableId").toInt();
    QString tableName = QString("%1 - %2").arg(QRKGastro::getRoomNameFromTableId(table)).arg(QRKGastro::getTableName(table));
    // print unprinted orders
    QSqlQuery orders(dbc);
    orders.prepare("SELECT (ticketorders.count - ticketorders.printed), products.name, ticketorders.id, ticketorders.product FROM ticketorders "
                   "LEFT JOIN products ON ticketorders.product=products.id "
                   "WHERE ticketorders.ticketId=:ticketId AND (ticketorders.count > ticketorders.printed)");
    orders.bindValue(":ticketId", ticket);
    bool ok = orders.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << orders.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(orders);
    }

    QSqlQuery orderDescription(dbc);
    orderDescription.prepare("SELECT description FROM orderDescs WHERE orderId=:id AND type=1");

    QJsonObject data;
    if ( orders.next() ) {
        data["customerText"] = tr("Bestellung %1").arg(tableName);

        //        if ( printer.outputFormat() != QPrinter::PdfFormat )
        //        {
        //          painter.translate(WIDTH, printer.pageRect().height());
        //          painter.rotate(180);
        //        }

        do {
            QString description = "";
            int ordersId = orders.value("id").toInt();
            orderDescription.bindValue(":id", ordersId);
            orderDescription.exec();
            if (orderDescription.next())
                description = orderDescription.value("description").toString();

            QJsonObject obj;
            QJsonArray arr;

            int printerid = Database::getPrinterIdFromProduct(orders.value("product").toInt());

            obj["product"] = orders.value(1).toString();
            obj["count"] = orders.value(0).toInt();
            if (!description.isEmpty())
                obj["description"] = description;

            qDebug() << QString::number(orders.value(0).toInt());
            qDebug() << orders.value(1).toString();


            // check for orderExtras
            QSqlQuery extras(dbc);
            extras.prepare("SELECT orderExtras.type, products.name FROM orderExtras "
                           " LEFT JOIN products ON orderExtras.product=products.id"
                           " WHERE orderId=:orderId");
            extras.bindValue(":orderId", orders.value(2).toInt());
            extras.exec();

            QJsonArray extra;
            while ( extras.next() ) {
                extra.append(QString((extras.value(0).toInt() == QRKGastro::TYPE_WITH) ? "+" : "-") + ' ' + extras.value(1).toString());
                qDebug() <<                              QString((extras.value(0).toInt() == QRKGastro::TYPE_WITH) ? "+" : "-") + ' ' + extras.value(1).toString();
            }
            obj["extra"] = extra;
            arr = data[QString::number(printerid)].toArray();
            arr.append(obj);
            data[QString::number(printerid)] = arr;
        }
        while ( orders.next() );
        if (!data.isEmpty()) {
            DocumentPrinter p(this);
            p.printTagged(data);
        }
        //        ui->openTickets->refreshTickets();
    }
}

bool QRKGastroTableOrder::finishOrder()
{
    if ( !QRKGastro::createOrUpdateTicket(ui->orderList, m_currentTicket, m_currentTable) )
        return false;

    //  ui->openTickets->refreshTickets();

    return true;
}

void QRKGastroTableOrder::plusSlot()
{
    QList<QTreeWidgetItem*> selected = ui->orderList->selectedItems();

    if ( selected.isEmpty() || selected[0]->parent() )  // only on toplevel items
        return;

    selected[0]->setData(0, Qt::DisplayRole, selected[0]->data(0, Qt::DisplayRole).toInt() + 1);
    m_history->historyInsertLine(tr("%1 ARTIKEL").arg("+"), tr("Artikel %1").arg(selected[0]->data(1, Qt::DisplayRole).toString()));

    updateOrderSum();
}

//--------------------------------------------------------------------------------

void QRKGastroTableOrder::minusSlot()
{
    QList<QTreeWidgetItem*> selected = ui->orderList->selectedItems();

    if ( selected.isEmpty() || selected[0]->parent() )  // only on toplevel items
        return;

    if ( selected[0]->data(0, Qt::DisplayRole).toInt() > 1 ) {
        if (getCountOfProduct(selected) > 0) {
            selected[0]->setData(0, Qt::DisplayRole, selected[0]->data(0, Qt::DisplayRole).toInt() - 1);
            m_history->historyInsertLine(tr("%1 ARTIKEL").arg("-"), tr("Artikel %1").arg(selected[0]->data(1, Qt::DisplayRole).toString()));
        } else {
            QRKGastroVoidDialog voidDialog(this);
            voidDialog.exec();
        }
    }

    updateOrderSum();
}

//--------------------------------------------------------------------------------

void QRKGastroTableOrder::removeSlot()
{
    QList<QTreeWidgetItem*> selected = ui->orderList->selectedItems();

    if ( selected.isEmpty() )
        return;

    if(getCountOfProduct(selected) < 1) {
        QRKGastroVoidDialog voidDialog(this);
        voidDialog.exec();
    }

    selected[0]->setHidden(true);

//    int x = ui->orderList->indexOfTopLevelItem(selected[0]);
//    QTreeWidgetItem *item = ui->orderList->takeTopLevelItem(x);

    if ( selected[0]->parent() == Q_NULLPTR ) {  // toplevel item (order item) was removed
        m_history->historyInsertLine(tr("%1 ARTIKEL").arg("Gelöscht"), tr("Artikel %1").arg(selected[0]->data(1, Qt::DisplayRole).toString()));
        updateOrderSum();
    }

//    delete item;
}

void QRKGastroTableOrder::withButtonSlot()
{
    if ( ui->withoutButton->isChecked() )
        ui->withoutButton->setChecked(false);
}

// only one of "with" our "without" may be checked
void QRKGastroTableOrder::withoutButtonSlot()
{
    if ( ui->withButton->isChecked() )
        ui->withButton->setChecked(false);
}

void QRKGastroTableOrder::quickProduct()
{
    QrkGastroQuickProduct qp(this);
    if (qp.exec() == QDialog::Accepted) {
        int id = qp.getProductId();
        emit addSelectedProduct(id);
    }
}

int QRKGastroTableOrder::getCountOfProduct(const QList<QTreeWidgetItem*> &selected)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery orders(dbc);
    int count = selected[0]->data(0, Qt::DisplayRole).toInt();
    orders.prepare("SELECT (ticketorders.count - ticketorders.printed) AS count, products.name, ticketorders.id FROM ticketorders "
                   " LEFT JOIN products ON ticketorders.product=products.id"
                   " WHERE ticketorders.ticketId=:id AND (ticketorders.count > ticketorders.printed)");
    orders.bindValue(":id", m_currentTicket);
    bool ok = orders.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << orders.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(orders);
    }
    if (orders.next())
        count -= orders.value("count").toInt();

    return count;
}

void QRKGastroTableOrder::writeSettings()
{
    QrkSettings settings;

    settings.beginGroup("Gastro");
    settings.save2Settings("splitterGeometry", ui->splitter->saveGeometry(), false);
    settings.save2Settings("splitterState", ui->splitter->saveState(), false);

    settings.endGroup();
}

void QRKGastroTableOrder::readSettings()
{
    QrkSettings settings;

    settings.beginGroup("Gastro");
    ui->splitter->restoreGeometry(settings.value("splitterGeometry").toByteArray());
    ui->splitter->restoreState(settings.value("splitterState").toByteArray());
    settings.endGroup();
}
