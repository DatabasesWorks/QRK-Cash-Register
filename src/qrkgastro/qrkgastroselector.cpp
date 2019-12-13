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

#include "qrkgastroselector.h"
#include "database.h"
#include "qrkgastromanagers/qrkgastrotablemanager.h"
#include "ui_qrkgastroselector.h"

#include <QSqlDatabase>
#include <QSqlQuery>

QRKGastroSelector::QRKGastroSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QRKGastroSelector)
{
    ui->setupUi(this);
    connect(ui->exitButton, &QPushButton::clicked, this, &QRKGastroSelector::cancelGastroButton_clicked);
    connect(ui->quickButtons, &QrkRoomTableButtons::tableOrder, this, &QRKGastroSelector::tableOrder);
    connect(ui->managerButton, &QPushButton::clicked, this, &QRKGastroSelector::manager);
}

QRKGastroSelector::~QRKGastroSelector()
{
    delete ui;
}

DragPushButton *QRKGastroSelector::getTableButton(int id)
{
    return ui->quickButtons->getTableButton(id);
}

void QRKGastroSelector::manager()
{
    QRKGastroTableManager tableManager(this);
    if (tableManager.exec() == QRKGastroTableManager::Rejected)
        refresh();
    else
        ui->quickButtons->refresh();
}

void QRKGastroSelector::refresh()
{
    if (getTableCount() == 0)
        manager();

    ui->quickButtons->refresh();
    // Table 0 for DirectPay
    // emit tableOrder(0);
}

int QRKGastroSelector::getTableCount()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.exec("SELECT count(id) AS count FROM tables");
    if (query.next())
        return query.value("count").toInt();

    return 0;
}
