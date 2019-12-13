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

#include "printersettingedit.h"
#include "printerdelegate.h"
#include "database.h"
#include "3rdparty/ckvsoft/jsontablemodel.h"
#include <ui_printersettingedit.h>

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QMessageBox>
#include <QPageSize>
#include <qprinterinfo.h>
#include <QDebug>

//--------------------------------------------------------------------------------
PrinterSettingEdit::PrinterSettingEdit(const QStringList &availablePrinters, QWidget *parent, int id)
    : QDialog(parent), ui(new Ui::PrinterSettingEdit), m_id(id)
{
    ui->setupUi(this);

    QSqlDatabase dbc = Database::database();
    QJsonTableModel::Header header;
    header.push_back( QJsonTableModel::Heading( { {"title","Id"},   {"index","id"} }) );
    header.push_back( QJsonTableModel::Heading( { {"title","Drucker"},   {"index","name"} }) );
    header.push_back( QJsonTableModel::Heading( { {"title","Definition"},  {"index","definition"} }) );

    m_model = new QJsonTableModel( header, this );
    m_model->enableColumnEdit(1, true);
    m_model->enableColumnEdit(2, true);

    if ( m_id != -1 ) {
        QSqlQuery query(dbc);
        query.prepare("SELECT id, name, printer FROM printers WHERE id=:id");
        query.bindValue(":id", id);
        query.exec();
        query.next();

        ui->name->setText(query.value("name").toString());
        m_name = query.value("name").toString();
        QByteArray data = QByteArray::fromBase64(query.value("printer").toByteArray());
        m_json = QJsonDocument().fromBinaryData(data).array();
        setDefinitionNames();
        m_model->setJson(m_json);
    }

    ui->tableView->setModel(m_model);
    ui->tableView->setItemDelegate(new PrinterDelegate(availablePrinters, this, true));
    ui->tableView->resizeColumnsToContents();
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableView->setColumnHidden(0, true);

    connect(ui->tableView, &QTableView::clicked, ui->tableView, static_cast<void(QTableView::*)(const QModelIndex &)>(&QTableView::edit));
    // connect(ui->tableView, &QTableView::doubleClicked, ui->tableView, static_cast<void(QTableView::*)(const QModelIndex &)>(&QTableView::edit));
    connect(ui->okButton, &QrkPushButton::clicked, this, &PrinterSettingEdit::accept);
    connect(ui->cancelButton, &QrkPushButton::clicked, this, &PrinterSettingEdit::close);
    connect(ui->newButton, &QrkPushButton::clicked, this, &PrinterSettingEdit::addNew);
    connect(ui->deleteButton, &QrkPushButton::clicked, this, &PrinterSettingEdit::remove);
    connect(ui->name, &QLineEdit::textChanged, this,&PrinterSettingEdit::textChanged);
}

PrinterSettingEdit::~PrinterSettingEdit()
{
    delete ui;
}

void PrinterSettingEdit::textChanged(const QString &text)
{
    if (Database::exists("printers", text, "name")) {
        QMessageBox::information(this, tr("Aliasname"), tr("Der Name %1 ist schon in Verwendung.").arg(text),
                                       QMessageBox::Ok);
        return;
    }
}

void PrinterSettingEdit::setDefinitionNames()
{
    QJsonObject printer;
    QJsonArray jArray;
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT name FROM printerdefs WHERE id=:id");
    int i = 0;
    foreach (const QJsonValue &value, m_json) {
        QJsonObject obj = value.toObject();
        query.bindValue(":id", obj["definitionid"].toInt());
        query.exec();
        query.next();
        printer["id"] = ++i;;
        printer["name"] = obj["name"].toString();
        printer["definitionid"] = obj["definitionid"].toInt();
        printer["definition"] = query.value("name").toString();
        jArray.append(printer);
    }
    m_json = jArray;
}

void PrinterSettingEdit::addNew()
{
    QJsonArray jArray;
    QJsonObject printer;
    int rowCount = m_model->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        printer["id"] = m_model->data(m_model->index(i, 0), Qt::DisplayRole).toString();
        printer["name"] = m_model->data(m_model->index(i, 1), Qt::DisplayRole).toString();
        printer["definition"] = m_model->getJsonObject(m_model->index(i, 2))["definition"].toString();
        printer["definitionid"] = Database::getDefinitionId(m_model->getJsonObject(m_model->index(i, 2))["definition"].toString());
        jArray << printer;
    }
    printer["id"] = m_model->rowCount() +1;
    jArray << printer;

    m_json = jArray;
    m_model->setJson(m_json);
}

void PrinterSettingEdit::remove()
{
    int row = ui->tableView->currentIndex().row();
    if ( row == -1 )
        return;

    if ( QMessageBox::question(this, tr("Drucker löschen"),
                               tr("Möchten sie den Drucker '%1' wirklich löschen?")
                               .arg(m_model->data(m_model->index(row, 1)).toString()),
                               QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
        return;

    QJsonArray jArray;
    QJsonObject printer;
    int id = m_model->data(m_model->index(row, 0), Qt::DisplayRole).toInt();
    for (int i = 0; i < m_model->rowCount(); ++i) {
        int currentid = m_model->data(m_model->index(i, 0), Qt::DisplayRole).toInt();
        if (currentid != id) {
            printer["id"] = m_model->data(m_model->index(i, 0), Qt::DisplayRole).toInt();
            printer["name"] = m_model->data(m_model->index(i, 1), Qt::DisplayRole).toString();
            printer["definition"] = m_model->getJsonObject(m_model->index(i, 2))["definition"].toString();
            printer["definitionid"] = Database::getDefinitionId(m_model->getJsonObject(m_model->index(i, 2))["definition"].toString());
            jArray << printer;
        }
    }

    m_json = jArray;
    m_model->setJson(m_json);

}

void PrinterSettingEdit::accept()
{

    QJsonArray jArray;
    QJsonObject printer;
    if (ui->name->text().isEmpty())
        return;

    if (m_id == -1 && Database::exists("printers", ui->name->text(), "name")) {
        QMessageBox::information(this, tr("Aliasname"), tr("Der Name %1 ist schon in Verwendung.").arg(ui->name->text()), QMessageBox::Ok);
        return;
    }

    for (int i = 0; i < m_model->rowCount(); ++i) {
        printer["name"] = m_model->data(m_model->index(i, 1), Qt::DisplayRole).toString();
        QString x = m_model->getJsonObject(m_model->index(i, 2))["definition"].toString();
        printer["definitionid"] = Database::getDefinitionId(m_model->getJsonObject(m_model->index(i, 2))["definition"].toString());
        jArray << printer;
    }
    m_json = jArray;

    updateData(m_id, ui->name->text());

    QDialog::accept();
}

//--------------------------------------------------------------------------------

bool PrinterSettingEdit::updateData(int id, const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    if ( id == -1 ) {
        query.prepare(QString("INSERT INTO printers (name, printer) VALUES(:name, :printer)"));
    } else {
        query.prepare(QString("UPDATE printers SET name=:name, printer=:printer WHERE id=:id"));
        query.bindValue(":id", id);
    }

    query.bindValue(":name", name);
    query.bindValue(":printer", QJsonDocument(m_json).toBinaryData().toBase64());

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    return ok;
}
