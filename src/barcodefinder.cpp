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

#include "barcodefinder.h"

#include "qrkdelegate.h"
#include "database.h"
#include "3rdparty/ckvsoft/numerickeypad.h"
#include "qsortfiltersqlquerymodel.h"
#include "preferences/qrksettings.h"
#include <ui_barcodefinder.h>

#include <QSqlRelationalTableModel>
#include <QSqlRelation>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QHeaderView>
#include <QModelIndex>
#include <QKeyEvent>

//--------------------------------------------------------------------------------

BarcodeFinder::BarcodeFinder(QWidget *parent)
    : QDialog(parent), ui(new Ui::BarcodeFinder)
{
//    setWindowFlags( Qt::WindowCloseButtonHint /* | Qt::WindowTitleHint*/);
    setWindowFlags(Qt::Dialog
    | Qt::WindowMinimizeButtonHint
    | Qt::WindowMaximizeButtonHint
    | Qt::WindowCloseButtonHint);

    ui->setupUi(this);
    keyPad = new NumericKeypad(false);
    keyPad->setVisible(true);
    keyPad->setFixedSize(keyPad->sizeHint());
    ui->gridLayout->addWidget(keyPad,1,0);
    ui->gridLayout->setAlignment(keyPad, Qt::AlignTop | Qt::AlignLeft);

    QSqlDatabase dbc = Database::database();

    m_proxyModel = new QSortFilterSqlQueryModel(this);
    m_proxyModel->setQuery("SELECT barcode, name FROM products", dbc);
    m_proxyModel->setFilterColumn("barcode");
    m_proxyModel->setFilter("");
    m_proxyModel->setFilterFlags(Qt::MatchStartsWith);
    m_proxyModel->select();

    ui->tableView->setModel(m_proxyModel);

    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    m_proxyModel->setHeaderData(0, Qt::Horizontal, tr("Barcode"), Qt::DisplayRole);
    m_proxyModel->setHeaderData(1, Qt::Horizontal, tr("Artikelname"), Qt::DisplayRole);

    connect(keyPad, &NumericKeypad::textChanged, m_proxyModel, &QSortFilterSqlQueryModel::filter);
    connect(ui->commitPushButton, &QPushButton::clicked, this, &BarcodeFinder::commitPushButton_clicked);
    connect(ui->tableView, &QTableView::activated, this, &BarcodeFinder::viewActivated);
    connect(ui->exitPushButton, &QPushButton::clicked, this, &BarcodeFinder::close);

    QrkSettings settings;
    settings.beginGroup("BarcodeReader");
    m_barcodeReaderPrefix = settings.value("barcodeReaderPrefix", Qt::Key_F11).toInt();
    settings.endGroup();
}

BarcodeFinder::~BarcodeFinder()
{
    delete ui;
}

void BarcodeFinder::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == m_barcodeReaderPrefix) {
        keyPad->setEditFocus();
    }
}

void BarcodeFinder::commitPushButton_clicked(bool)
{
    emit barcodeCommit(m_barcode);
    keyPad->clear();
    m_barcode.clear();
    ui->commitPushButton->setEnabled(false);
}

void BarcodeFinder::viewActivated(QModelIndex idx)
{
    ui->commitPushButton->setEnabled(true);
    m_barcode = m_proxyModel->data(m_proxyModel->index(idx.row(), 0), Qt::DisplayRole).toString();
}
