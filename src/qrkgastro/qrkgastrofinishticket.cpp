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

#include "qrkgastrofinishticket.h"
#include "qrkgastrofinishticketdialog.h"
#include "receiptitemmodel.h"
#include "reports.h"
#include "givendialog.h"
#include "preferences/qrksettings.h"
#include "qrktimedmessagebox.h"
#include "qrkprogress.h"
#include "database.h"
#include "defines.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>

QRKGastroFinishTicket::QRKGastroFinishTicket(QWidget *parent)
    :QWidget(parent), m_receiptitemmodel(new ReceiptItemModel(this))
{
    QrkSettings settings;
    m_useGivenDialog = settings.value("useGivenDialog", false).toBool();
    m_receiptPrintDialog = settings.value("useReceiptPrintedDialog", false).toBool();
}

QRKGastroFinishTicket::~QRKGastroFinishTicket()
{

}

bool QRKGastroFinishTicket::createReceipt(int ticket)
{
    /*
     * {"receipt":[
     *  {"customertext": "Customer Text",
     *   "payedBy": "0",
     *   "items":[
     *     { "count": "3", "name": "Kupplung", "gross": "122,70", "tax": "20" },
     *     { "count": "1", "name": "Bremsbeläge", "gross": "32,30", "tax": "10" },
     *     { "count": "2", "name": "Benzinschlauch", "gross": "17,80", "tax": "20" },
     *     { "count": "1", "name": "Ölfilter", "gross": "104,50", "tax": "13" }
     *    ]
     *   }
     * ]}
*/
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    // calc sum
    query.prepare("SELECT SUM(count * gross) as gross FROM ticketorders WHERE ticketId=:ticketId");
    query.bindValue(":ticketId", ticket);
    query.exec();
    query.next();
    QBCMath sum(query.value("gross").toDouble());
    sum.round(2);

    QRKGastroFinishTicketDialog question(sum, this);

    int code = question.exec();
    if ( code == QDialog::Rejected )
        return false;

    QJsonObject Root;//root object
    QJsonArray main;
    QJsonObject mainitem;
    QJsonArray items;

    query.prepare("SELECT open FROM tickets WHERE id=:ticketId");
    query.bindValue(":ticketId", ticket);
    query.exec();
    query.next();

    QDateTime invoiceTime = QDateTime::currentDateTime();

    int payedBy = 0;

    if (code == QRKGastroFinishTicketDialog::CASHRECEIPT_TICKET)
        payedBy = PAYED_BY_CASH;
    else if (code == QRKGastroFinishTicketDialog::CREDITCARD_TICKET)
        payedBy = PAYED_BY_CREDITCARD;
    else if (code == QRKGastroFinishTicketDialog::DEBITCARD_TICKET)
        payedBy = PAYED_BY_DEBITCARD;
    else if (code == QRKGastroFinishTicketDialog::PRIVATE_TICKET)
        payedBy = PAYED_BY_PRIVATE;
    else if (code == QRKGastroFinishTicketDialog::EMPLOYEE_TICKET)
        payedBy = PAYED_BY_EMPLOYEE;
    else if (code == QRKGastroFinishTicketDialog::ADVERTISING_TICKET)
        payedBy = PAYED_BY_ADVERTISING;

    mainitem["payedBy"] = QString::number(payedBy);

 //   mainitem["customertext"] = "Customer Text";
    QSqlQuery orders(dbc);

    orders.prepare("SELECT ticketorders.count, products.name, products.tax, ticketorders.gross, ticketorders.id FROM ticketorders "
                   " LEFT JOIN products ON ticketorders.product=products.id"
                   " WHERE ticketorders.ticketId=:id");
    orders.bindValue(":id", ticket);
    orders.exec();

    while(orders.next()) {
        QJsonObject item;
        QBCMath gross(orders.value("gross").toDouble());
        gross.round(2);
        QBCMath tax(orders.value("tax").toDouble());
        tax.round(2);
        item["count"] = orders.value("count").toString();
        item["name"] = orders.value("name").toString();
        item["gross"] = (payedBy < 0)? "0.00": gross.toString();
        item["tax"] = tax.toString();
        items.append(item);
    }
    mainitem["items"] = items;
    main.append(mainitem);
    Root.insert("receipt", main);
    QJsonDocument doc(Root);

    m_receiptitemmodel->newOrder();
    if (m_receiptitemmodel->setReceiptServerMode(mainitem)) {
        if (m_useGivenDialog && payedBy == PAYED_BY_CASH) {
            double s = sum.toDouble();
            GivenDialog given(s, this);
            if (given.exec() == 0) {
                return false;
            }
            m_receiptitemmodel->setGiven(given.getGiven());
        }
        if (payNow(payedBy)) {
            query.prepare("UPDATE tickets SET timestamp=:timestamp, open=0, payedBy=:payedBy WHERE id=:ticketId");
            query.bindValue(":timestamp",invoiceTime.toString(Qt::ISODate));
            query.bindValue(":ticketId", ticket);
            query.bindValue(":payedBy", payedBy);
            query.exec();
            return true;
        }
    }
    return false;
}

bool QRKGastroFinishTicket::payNow(int payedBy)
{

    QDateTime dateTime;
    dateTime = QDateTime::currentDateTime();

    QRKProgress waitBar;
    waitBar.setText(tr("Beleg wird erstellt."));
    waitBar.setWaitMode();
    waitBar.show();

    Reports rep;
    bool ret = rep.checkEOAny(dateTime);
    if (!ret) {
        return false;
    }

//    emit sendDatagram("topay", ui->sumLabel->text().replace(Database::getCurrency() ,""));

    if (m_receiptitemmodel->rowCount() > 0)
    {
        int rc = m_receiptitemmodel->rowCount();

        for(int row = 0; row < rc; row++) {
            QJsonObject itemdata;
            itemdata["name"] = m_receiptitemmodel->data(m_receiptitemmodel->index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString();
            itemdata["tax"] = m_receiptitemmodel->data(m_receiptitemmodel->index(row, REGISTER_COL_TAX, QModelIndex())).toDouble();
            itemdata["net"] =  (payedBy < 0)?0.0:m_receiptitemmodel->data(m_receiptitemmodel->index(row, REGISTER_COL_NET, QModelIndex())).toDouble();
            itemdata["gross"] = (payedBy < 0)?0.0:m_receiptitemmodel->data(m_receiptitemmodel->index(row, REGISTER_COL_SINGLE, QModelIndex())).toDouble();
            itemdata["itemnum"] = m_receiptitemmodel->data(m_receiptitemmodel->index(row, REGISTER_COL_PRODUCTNUMBER, QModelIndex())).toString();
            itemdata["visible"] = 1;

            Database::addProduct(itemdata);
        }
    }

    QSqlDatabase dbc = Database::database();
    dbc.transaction();

    m_currentReceipt = m_receiptitemmodel->createReceipts();
    bool sql_ok = true;
    if ( m_currentReceipt ) {
        if ( m_receiptitemmodel->createOrder() ) {
            if ( finishReceipts(payedBy) ){
                sql_ok = true;
            } else {
                sql_ok = false;
            }
        } else {
            sql_ok = false;
        }
    }

    if (sql_ok) {
        dbc.commit();
        if (m_receiptPrintDialog) {
            QrkTimedMessageBox messageBox(10,
                                          QMessageBox::Information,
                                          tr("Drucker"),
                                          tr("Beleg %1 wurde gedruckt. Nächster Vorgang wird gestartet.").arg(m_currentReceipt),
                                          QMessageBox::Yes | QMessageBox::Default
                                          );

            messageBox.setDefaultButton(QMessageBox::Yes);
            messageBox.setButtonText(QMessageBox::Yes, QObject::tr("OK"));
            messageBox.exec();
        }
    } else {
        sql_ok = dbc.rollback();
        QMessageBox::warning(this, tr("Fehler"), tr("Datenbank und/oder Signatur Fehler!\nAktueller BON kann nicht erstellt werden. (Rollback: %1).\nÜberprüfen Sie ob genügend Speicherplatz für die Datenbank vorhanden ist. Weitere Hilfe gibt es im Forum. http:://www.ckvsoft.at").arg(sql_ok?tr("durchgeführt"):tr("fehlgeschlagen") ));
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << dbc.lastError().text();
    }

//    emit sendDatagram("finished", "");

    return true;
}

bool QRKGastroFinishTicket::finishReceipts(int payedBy, int id, bool isReport)
{
    QDateTime dt = QDateTime::currentDateTime();;
//    m_receiptitemmodel->setCustomerText(ui->customerText->text());
    m_receiptitemmodel->setReceiptTime(dt);
    bool ret = m_receiptitemmodel->finishReceipts(payedBy, id, isReport);
    return ret;
}
