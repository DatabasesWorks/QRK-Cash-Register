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

#include "printerdefinitionedit.h"
#include "database.h"
#include <ui_printerdefinitionedit.h>

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QPageSize>
#include <QDebug>

//--------------------------------------------------------------------------------
PrinterDefinitionEdit::PrinterDefinitionEdit(QWidget *parent, int id)
    : QDialog(parent), ui(new Ui::PrinterDefinitionEdit), m_id(id)
{
    ui->setupUi(this);

    QSqlDatabase dbc = Database::database();

    if ( m_id != -1 ) {
        QSqlQuery query(QString("SELECT id, name, definition FROM printerdefs WHERE id=%1").arg(id), dbc);
        query.next();

        ui->paperHeightSpinBox->setMaximum(4000);

        ui->name->setText(query.value("name").toString());
        m_name = query.value("name").toString();
        QByteArray data = QByteArray::fromBase64(query.value("definition").toByteArray());
        m_json = QJsonDocument().fromBinaryData(data).object();
        ui->pdfCheckBox->setChecked(m_json["pdf"].toBool());
        QString type = m_json["type"].toString();
        if (type == "custom") {
            typeChanged(0);
        } else {
            typeChanged(type);
        }

        ui->marginLeftSpinBox->setValue(m_json["marginLeft"].toInt());
        ui->marginRightSpinBox->setValue(m_json["marginRight"].toInt());
        ui->marginTopSpinBox->setValue(m_json["marginTop"].toInt());
        ui->marginBottomSpinBox->setValue(m_json["marginBottom"].toInt());
    } else {
        typeChanged("A4");
    }

    connect(ui->groupComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, static_cast<void(PrinterDefinitionEdit::*)(int)>(&PrinterDefinitionEdit::typeChanged));
    connect(ui->paperWidthSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrinterDefinitionEdit::paperWidthChanged);
    connect(ui->paperHeightSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrinterDefinitionEdit::paperHeightChanged);
    connect(ui->marginTopSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrinterDefinitionEdit::marginTopChanged);
    connect(ui->marginLeftSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrinterDefinitionEdit::marginLeftChanged);
    connect(ui->marginRightSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrinterDefinitionEdit::marginRightChanged);
    connect(ui->marginBottomSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PrinterDefinitionEdit::marginBottomChanged);
    connect(ui->pdfCheckBox, &QCheckBox::clicked, this, &PrinterDefinitionEdit::pdfChanged);
    connect(ui->okButton, &QrkPushButton::clicked, this, &PrinterDefinitionEdit::accept);
    connect(ui->cancelButton, &QrkPushButton::clicked, this, &PrinterDefinitionEdit::close);
}

PrinterDefinitionEdit::~PrinterDefinitionEdit()
{
    delete ui;
}

void PrinterDefinitionEdit::typeChanged(int idx)
{
    if (idx == 0) {
        ui->paperWidthSpinBox->setEnabled(true);
        ui->paperHeightSpinBox->setEnabled(true);
        ui->paperWidthSpinBox->setValue(m_json["paperWidth"].toInt());
        ui->paperHeightSpinBox->setValue(m_json["paperHeight"].toInt());
        ui->groupComboBox->setCurrentIndex(0);
    } else {
        typeChanged(ui->groupComboBox->currentText());
    }
}

void PrinterDefinitionEdit::typeChanged(const QString &text)
{
    ui->paperWidthSpinBox->setEnabled(false);
    ui->paperHeightSpinBox->setEnabled(false);
    QSizeF size = QPageSize::size((text == "A4")?QPageSize::A4:QPageSize::A5, QPageSize::Millimeter);
    ui->paperWidthSpinBox->setMaximum(int(size.width()));
    ui->paperWidthSpinBox->setValue(int(size.width()));
    ui->paperHeightSpinBox->setValue(int(size.height()));
    ui->groupComboBox->setCurrentText(text);
}

void PrinterDefinitionEdit::paperHeightChanged(int height)
{
    if (ui->groupComboBox->currentIndex() == 0)
        m_json["paperHeight"] = height;
}

void PrinterDefinitionEdit::paperWidthChanged(int width)
{
    if (ui->groupComboBox->currentIndex() == 0)
        m_json["paperWidth"] = width;
}

void PrinterDefinitionEdit::marginTopChanged(int margin)
{
    m_json["marginTop"] = margin;
}

void PrinterDefinitionEdit::marginLeftChanged(int margin)
{
    m_json["marginLeft"] = margin;
}

void PrinterDefinitionEdit::marginRightChanged(int margin)
{
    m_json["marginRight"] = margin;
}

void PrinterDefinitionEdit::marginBottomChanged(int margin)
{
    m_json["marginBottom"] = margin;
}

void PrinterDefinitionEdit::pdfChanged(bool checked)
{
    m_json["pdf"] = checked;
}

void PrinterDefinitionEdit::accept()
{

    if (ui->name->text().isEmpty())
        return;

    m_json["pdf"] = ui->pdfCheckBox->isChecked();
    m_json["marginBottom"] = ui->marginBottomSpinBox->value();
    m_json["marginRight"] = ui->marginRightSpinBox->value();
    m_json["marginLeft"] = ui->marginLeftSpinBox->value();
    m_json["marginTop"] = ui->marginTopSpinBox->value();
    m_json["paperWidth"] = ui->paperWidthSpinBox->value();
    m_json["paperHeight"] = ui->paperHeightSpinBox->value();
    if (ui->groupComboBox->currentIndex() == 0) {
        m_json["type"] = "custom";
    } else {
        m_json["type"] = ui->groupComboBox->currentText();
    }

    updateData(m_id, ui->name->text());

    QDialog::accept();
}

//--------------------------------------------------------------------------------

bool PrinterDefinitionEdit::updateData(int id, const QString &name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    if ( id == -1 ) {
        query.prepare(QString("INSERT INTO printerdefs (name, definition) VALUES(:name, :definition)"));
    } else {
        query.prepare(QString("UPDATE printerdefs SET name=:name, definition=:definition WHERE id=:id"));
        query.bindValue(":id", id);
    }

    query.bindValue(":name", name);
    query.bindValue(":definition", QJsonDocument(m_json).toBinaryData().toBase64());

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    return ok;
}
