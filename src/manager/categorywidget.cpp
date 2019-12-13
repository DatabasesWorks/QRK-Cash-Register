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

#include "categoryedit.h"
#include "categorywidget.h"
#include "database.h"
#include "ui_categorywidget.h"

#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>

//--------------------------------------------------------------------------------

CategoryWidget::CategoryWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::CategoryWidget)

{
    ui->setupUi(this);

    QSqlDatabase dbc = Database::database();

    connect(ui->plus, &QPushButton::clicked, this, &CategoryWidget::plusSlot);
    connect(ui->minus, &QPushButton::clicked, this, &CategoryWidget::minusSlot);
    connect(ui->edit, &QPushButton::clicked, this, &CategoryWidget::editSlot);
    connect(ui->groupFilter, &QLineEdit::textChanged, this, &CategoryWidget::filterGroup);

    m_model = new QSqlTableModel(this, dbc);
    m_model->setTable("categories");
//    m_model->setFilter("id > 1");
    m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_model->select();

    m_model->setHeaderData(m_model->fieldIndex("name"), Qt::Horizontal, tr("Kategorie"), Qt::DisplayRole);
    m_model->setHeaderData(m_model->fieldIndex("visible"), Qt::Horizontal, tr("sichtbar"), Qt::DisplayRole);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(m_model->fieldIndex("name"));

    ui->tableView->setModel(m_proxyModel);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->setColumnHidden(m_model->fieldIndex("id"), true);
    ui->tableView->setColumnWidth(m_model->fieldIndex("name"), 250);
    ui->tableView->setColumnHidden(m_model->fieldIndex("color"), true);
    ui->tableView->setColumnHidden(m_model->fieldIndex("printerid"), true);
    ui->tableView->setColumnHidden(m_model->fieldIndex("image"), true);
    ui->tableView->setColumnHidden(m_model->fieldIndex("sortorder"), true);

    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->resizeColumnsToContents();
    //  ui->tableView->horizontalHeader()->setSectionResizeMode(model->fieldIndex("name"), QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->verticalHeader()->setVisible(false);

}

CategoryWidget::~CategoryWidget()
{
    delete ui;
}

//--------------------------------------------------------------------------------

void CategoryWidget::filterGroup(const QString &filter)
{
    // show only matching items

    m_proxyModel->setFilterWildcard("*" + filter + "*");

    m_model->fetchMore();  // else the list is not filled with all possible rows
    // e.g. when using mouse wheel it would fetch more items
    // but on the WeTab we have no mouse
}

//--------------------------------------------------------------------------------

void CategoryWidget::plusSlot()
{
    CategoryEdit dialog(this);
    dialog.exec();

    m_model->select();
}

//--------------------------------------------------------------------------------

void CategoryWidget::minusSlot()
{
    int row = m_proxyModel->mapToSource(ui->tableView->currentIndex()).row();
    if ( row == -1 )
        return;

    int id = m_model->data(m_model->index(row, m_model->fieldIndex("id"))).toInt();
    if (id < 3) {
        QMessageBox::information(this, tr("Löschen nicht möglich"),
                                 tr("Standard Kategorie '%1' kann nicht gelöscht werden. Der Kategoriename kann geändert werden.")
                                 .arg(m_model->data(m_model->index(row, 1)).toString()));

        return;
    }

    if ( QMessageBox::question(this, tr("Kategorie löschen"),
                               tr("Möchten sie die Kategorie '%1' wirklich löschen?\n\nGruppen die dieser Kategorie zugeordnet sind werden der Standard-Kategorie zugeordnet.")
                               .arg(m_model->data(m_model->index(row, 1)).toString()),
                               QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
        return;

    Database::moveGroupsToDefaultCategory(id);

    m_model->removeRow(row);
    m_model->select();
}

//--------------------------------------------------------------------------------

void CategoryWidget::editSlot()
{

    QModelIndex current(m_proxyModel->mapToSource(ui->tableView->currentIndex()));
    int row = current.row();
    if ( row == -1 )
        return;

    CategoryEdit dialog(this, m_model->data(m_model->index(row, m_model->fieldIndex("id"))).toInt());
    if ( dialog.exec() == QDialog::Accepted )
    {
        m_model->select();
        ui->tableView->resizeRowsToContents();
        ui->tableView->setCurrentIndex(current);
    }
}

//--------------------------------------------------------------------------------
