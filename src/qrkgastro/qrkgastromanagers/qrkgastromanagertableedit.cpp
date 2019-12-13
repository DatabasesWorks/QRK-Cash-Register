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

#include "qrkgastromanagertableedit.h"
#include "database.h"
#include "utils/utils.h"
#include "preferences/qrksettings.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "ui_qrkgastromanagertableedit.h"

#include <QDoubleValidator>
#include <QSqlRelationalTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

//--------------------------------------------------------------------------------
QRKGastroManagerTableEdit::QRKGastroManagerTableEdit(QWidget *parent, int roomId, int id)
    : QDialog(parent), ui(new Ui::QRKGastroManagerTableEdit), m_roomId(roomId), m_id(id)
{
    ui->setupUi(this);

    const QStringList colorNames = QColor::colorNames();
    int index = 0;
    ui->colorComboBox->addItem(tr("gleich wie Raum"));
    const QModelIndex idx = ui->colorComboBox->model()->index(index++, 0);
    ui->colorComboBox->model()->setData(idx, "", Qt::BackgroundColorRole);

    foreach (const QString &colorName, colorNames) {
        if (colorName == "green")
            continue;
        const QColor color(colorName);
        QString fg = Utils::color_best_contrast(color.name());
        const QColor fg_color(fg);
        ui->colorComboBox->addItem(colorName, color);
        const QModelIndex idx = ui->colorComboBox->model()->index(index++, 0);
        ui->colorComboBox->model()->setData(idx, color, Qt::BackgroundColorRole);
        ui->colorComboBox->model()->setData(idx, fg_color, Qt::ForegroundRole);
    }

    QrkSettings settings;

    QDoubleValidator *doubleVal = new QDoubleValidator(0.0, 9999999.99, 2, this);
    doubleVal->setNotation(QDoubleValidator::StandardNotation);

    QSqlDatabase dbc = Database::database();

    m_roomsModel = new QSqlRelationalTableModel(this, dbc);
    m_roomsModel->setQuery("SELECT id, name FROM rooms", dbc);
    ui->groupComboBox->setModel(m_roomsModel);
    ui->groupComboBox->setModelColumn(1);  // show name

    if ( m_id != -1 ) {
        QSqlQuery query(QString("SELECT `name`, `roomId`, `color` FROM tables WHERE id=%1").arg(id), dbc);
        query.next();

        ui->name->setText(query.value("name").toString());
        m_name = query.value("name").toString();

        int i;

        for (i = 0; i <= ui->colorComboBox->count(); i++) {
            QString color = ui->colorComboBox->model()->index(i, 0).data(Qt::BackgroundColorRole).toString();
            if ( query.value("color").toString() == color )
                break;
        }

        if (i > ui->colorComboBox->count())
            i = 0;

        QString colorValue = query.value("color").toString().trimmed();
        if (!colorValue.isEmpty()) {
            QPalette palette(ui->colorComboBox->palette());
            QColor color(colorValue);
            palette.setColor(QPalette::Active,QPalette::Button, color);
            palette.setColor(QPalette::Highlight, color);
            ui->colorComboBox->setPalette(palette);
        }
        ui->colorComboBox->setCurrentIndex(i);

    }

    for (int i = 0; i < m_roomsModel->rowCount(); i++) {
        if ( m_roomId == m_roomsModel->data(m_roomsModel->index(i, 0), Qt::DisplayRole).toInt() ) {
            ui->groupComboBox->setCurrentIndex(i);
            break;
        }
    }

    connect (ui->colorComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &QRKGastroManagerTableEdit::colorComboChanged);
    connect (ui->okButton, &QPushButton::clicked, this, &QRKGastroManagerTableEdit::accept);
    connect (ui->cancelButton, &QPushButton::clicked, this, &QRKGastroManagerTableEdit::reject);
    connect(ui->name, &QLineEdit::textChanged,this,&QRKGastroManagerTableEdit::textChanged);

    settings.endGroup();

}

QRKGastroManagerTableEdit::~QRKGastroManagerTableEdit()
{
    delete ui;
}

void QRKGastroManagerTableEdit::textChanged(const QString text)
{
    int id = m_roomsModel->data(m_roomsModel->index(ui->groupComboBox->currentIndex(), 0)).toInt();
    if (existTableinRoom(text, id) && m_name.compare(text) != 0) {
        QMessageBox::information(this, tr("Tischname"), tr("Der Name %1 ist schon in Verwendung.").arg(text),
                                       QMessageBox::Ok);
        return;
    }
}

void QRKGastroManagerTableEdit::colorComboChanged(int idx)
{

    QString colorValue = ui->colorComboBox->itemData(idx, Qt::BackgroundColorRole).toString(); // ->itemText(idx); //   ->model()->index(ui->colorComboBox->currentIndex(), 0).data(Qt::BackgroundColorRole).toString();
    QPalette palette(ui->colorComboBox->palette());
    QColor color(colorValue);
    palette.setColor(QPalette::Active,QPalette::Button, color);
    palette.setColor(QPalette::Highlight, color);
    ui->colorComboBox->setPalette(palette);
}

void QRKGastroManagerTableEdit::accept()
{

    if (ui->name->text().isEmpty())
        return;

    updateData(m_id, ui->name->text());

    QDialog::accept();
}

//--------------------------------------------------------------------------------

bool QRKGastroManagerTableEdit::updateData(int id, QString name)
{

    int roomId = m_roomsModel->data(m_roomsModel->index(ui->groupComboBox->currentIndex(), 0)).toInt();
    if (existTableinRoom(name, roomId)) {
        QMessageBox::information(this, tr("Tischname"), tr("Der Name %1 ist schon in Verwendung.").arg(name),
                                       QMessageBox::Ok);
        return false;
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    QString color = ui->colorComboBox->model()->index(ui->colorComboBox->currentIndex(), 0).data(Qt::BackgroundColorRole).toString();

    if ( id == -1 )  // new entry
    {
        query.prepare(QString("INSERT INTO tables (name, roomid, color) VALUES(:name, :room, :color)"));
    } else {
        query.prepare(QString("UPDATE table SET name=:name, roomid=:room, color=:color WHERE id=:id"));
        query.bindValue(":id", id);
    }

    query.bindValue(":name", name);
    query.bindValue(":room", roomId);
    query.bindValue(":color", color);

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    return ok;
}

bool QRKGastroManagerTableEdit::existTableinRoom(const QString tablename, int roomId)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT id FROM tables WHERE name=:tablename AND roomId=:roomId");
    query.bindValue(":roomId", roomId);
    query.bindValue(":tablename", tablename);
    query.exec();
    if (query.next()) {
        return true;
    }
    return false;
}
