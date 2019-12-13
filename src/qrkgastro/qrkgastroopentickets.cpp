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

#include "qrkgastro.h"
#include "qrkgastroopentickets.h"
#include "qrkgastrofinishticket.h"
#include "database.h"
#include "qrkgastrotableorder.h"
#include "qrkgastrosplitticketwidget.h"
#include "qrkgastrofinishticketdialog.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "history.h"
#include "reports.h"
#include "ui_qrkgastroopentickets.h"

#include <QTreeWidget>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

QRKGastroOpenTickets::QRKGastroOpenTickets(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QRKGastroOpenTickets)
{
    ui->setupUi(this);
    connect(ui->newTicket,&QrkPushButton::clicked, this, static_cast<void(QRKGastroOpenTickets::*)(bool)>(&QRKGastroOpenTickets::newTicket));
    connect(ui->voidTicket, &QrkPushButton::clicked, this, &QRKGastroOpenTickets::voidTicket);
    connect(ui->changeTicket, &QrkPushButton::clicked, this, static_cast<void(QRKGastroOpenTickets::*)(bool)>(&QRKGastroOpenTickets::changeTicket));
    connect(ui->splitTicket, &QrkPushButton::clicked, this, &QRKGastroOpenTickets::splitTicket);
    connect(ui->moveTicket, &QrkPushButton::clicked, this, &QRKGastroOpenTickets::moveTicket);
    connect(ui->leaveTicket, &QrkPushButton::clicked, this, &QRKGastroOpenTickets::leaveTicket);
    connect(ui->payTicket, &QPushButton::clicked, this, static_cast<void(QRKGastroOpenTickets::*)()>(&QRKGastroOpenTickets::payTicket));
    connect(ui->openTickets, &QrkGastroOpenTicketsListWidget::selectionChanged, this, &QRKGastroOpenTickets::selectionChanged);
}

QRKGastroOpenTickets::~QRKGastroOpenTickets()
{
    delete ui;
}

void QRKGastroOpenTickets::selectionChanged()
{
    QList<int> selected = ui->openTickets->getSelectedTickets();

    if ( selected.count() == 0 ) {
        ui->changeTicket->setEnabled(false);
        ui->payTicket->setEnabled(false);
        ui->splitTicket->setEnabled(false);
        ui->voidTicket->setEnabled(false);
        ui->moveTicket->setEnabled(false);
    } else {
        ui->changeTicket->setEnabled(true);
        ui->payTicket->setEnabled(true);
        ui->splitTicket->setEnabled(true);
        ui->voidTicket->setEnabled(true);
        ui->moveTicket->setEnabled(true);
    }
}

int QRKGastroOpenTickets::setTableId(int id)
{

    m_currentTable = id;
    ui->openTickets->refreshTickets(m_currentTable);
    int size = ui->openTickets->getTickets().size();
    if (size == 0) {
        if (Reports().mustDoEOAny(QDateTime::currentDateTime()))
            infoMessage();
        else
            emit newTicket(m_currentTable);
    }

    emit selectionChanged();
    return size;
}

void QRKGastroOpenTickets::newTicket(bool clicked)
{
    Q_UNUSED(clicked);
    if (Reports().mustDoEOAny(QDateTime::currentDateTime())) {
        infoMessage();
        return;
    }
    History history;
    history.historyInsertLine(tr("Bestellung"), tr("Bestellung neu Tisch %1").arg(QRKGastro::getTableName(m_currentTable)));

    emit newTicket(m_currentTable);
}

void QRKGastroOpenTickets::changeTicket(bool clicked)
{
    Q_UNUSED(clicked);

    int tableId = 0;
    int ticketId = 0;

    getSelectedTicket(ticketId, tableId);
    if ( ticketId == 0 )
        return;

    History history;
    history.historyInsertLine(tr("Bestellung"), tr("Bestellung ändern Tisch %1").arg(QRKGastro::getTableName(tableId)));

    emit changeTicket(tableId, ticketId);
}

void QRKGastroOpenTickets::voidTicket()
{
    if(!RBAC::Instance()->hasPermission("gastro_void_ticked", true)) {
        return;
    }

    int ticketId, tableId;
    getSelectedTicket(ticketId, tableId);
    if ( ticketId == 0 )
        return;

    if ( QMessageBox::question(this, tr("Bon löschen"), tr("Möchten sie diesen Bon wirklich löschen ?"),
                               QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    // remove ticket
    query.prepare("DELETE FROM orderExtras WHERE orderId IN (SELECT id FROM ticketorders WHERE ticketId=:ticketId)");
    query.bindValue(":ticketId", ticketId);
    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
    query.prepare("DELETE FROM ticketorders WHERE ticketId=:ticketId");
    query.bindValue(":ticketId", ticketId);
    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
    query.prepare("DELETE FROM tickets WHERE id=:ticketId");
    query.bindValue(":ticketId", ticketId);
    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    History history;
    history.historyInsertLine(tr("Bestellung"), tr("Bestellung stornieren Tisch %1").arg(QRKGastro::getTableName(m_currentTable)));

    // remove from display
    refresh();
}

void QRKGastroOpenTickets::getSelectedTicket(int &id, int &table)
{
    id = table = 0;

    QList<int> tickets = ui->openTickets->getTickets();
    QList<int> selected =  ui->openTickets->getSelectedTickets();

    if ( selected.count() > 1 )
    {
        QMessageBox::information(this, tr("Mehrfache Selektion"), tr("Nur ein Bon darf selektiert sein"));
        return;
    }

    // if there is only 1 ticket, use this even if not selected
    if ( tickets.count() == 1 )
    {
        id = tickets[0];
        table = ui->openTickets->getTableOfTicket(id);
        return;
    }

    if ( selected.count() == 1 )
    {
        id = selected[0];
        table = ui->openTickets->getTableOfTicket(id);
        return;
    }
}

void QRKGastroOpenTickets::moveTicket()
{
    emit splitTicket(true);
}

void QRKGastroOpenTickets::splitTicket(bool moveticket)
{
    int ticket, table;
    getSelectedTicket(ticket, table);
    if ( ticket == 0 )
        return;

    QRKGastroSplitTicketWidget splitWidget(moveticket, this);
    splitWidget.show();

    if ( splitWidget.exec(ticket, table) == QRKGastroSplitTicketWidget::SPLITTED ) {
        ui->openTickets->refreshTickets(m_currentTable);

        // select the new ticket
        ui->openTickets->selectTicket(ticket);
        History history;
        if (moveticket)
            history.historyInsertLine(tr("Bestellung"), tr("Bestellung auf Tisch %1 verschoben").arg(QRKGastro::getTableName(table)));
        else
            history.historyInsertLine(tr("Bestellung"), tr("Bestellung separieren Tisch %1").arg(QRKGastro::getTableName(m_currentTable)));
    }
}

bool QRKGastroOpenTickets::finishTicket(int ticket)
{

    QRKGastroFinishTicket finish(this);
    finish.createReceipt(ticket);

    ui->openTickets->refreshTickets(m_currentTable);

    //if ( code == QRKGastroFinishTicketDialog::WITH_PRINTOUT )
    //  printInvoice(ticket);
    History history;
    history.historyInsertLine(tr("Bestellung"), tr("Bestellung bezahlen Tisch %1").arg(QRKGastro::getTableName(m_currentTable)));

    return true;
}

void QRKGastroOpenTickets::payTicket(int tickeId)
{
    ui->openTickets->selectTicket(tickeId);
    emit payTicket();
}

void QRKGastroOpenTickets::payTicket()
{
  QList<int> selected = ui->openTickets->getSelectedTickets();

  if ( selected.count() == 0 )
      return;

  if ( selected.count() > 1 ) {
    payGroupTicket(selected);
    refresh();
    return;
  }

  QList<int> tickets = ui->openTickets->getTickets();
  if ( tickets.count() == 1 )  // if there's only 1, use this
    finishTicket(tickets[0]);
  else if ( selected.count() == 1 )  // if there are more, use only if 1 is selected
    finishTicket(selected[0]);

  refresh();
}

void QRKGastroOpenTickets::payGroupTicket(QList<int> tickets)
{
  if ( tickets.count() <= 1 )
    return;

  if ( QMessageBox::question(this, tr("Bons zusammenlegen"),
                             tr("Möchten sie diese %1 Bons wirklich zusammenlegen ?").arg(tickets.count()),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
    return;

  QSqlDatabase dbc = Database::database();
  QSqlQuery query(dbc);

  int firstId = tickets[0];
  for (int i = 1; i < tickets.count(); i++)
  {
    query.prepare("UPDATE ticketorders set ticketId=:firstId WHERE ticketId=ticketId");
    query.bindValue(":firstId", firstId);
    query.bindValue(":ticketId", tickets[i]);
    query.exec();
    query.prepare("DELETE FROM tickets where id=:ticketId");
    query.bindValue(":ticketId", tickets[i]);
    query.exec();
  }

  // update for merged ticket
  ui->openTickets->refreshTickets(m_currentTable);

  // select the merged one
  ui->openTickets->selectTicket(firstId);

  finishTicket(firstId);
}

void QRKGastroOpenTickets::refresh()
{
    ui->openTickets->refreshTickets(m_currentTable);
    if (ui->openTickets->getTickets().count() == 0)
        emit newTicket(m_currentTable);
//        emit leaveTicket();
}

void QRKGastroOpenTickets::infoMessage()
{
    QMessageBox::information(this, tr("Fehlender Abschluss"), tr("Leider ist keine neue Bonierung möglich."), QMessageBox::Ok);
}
