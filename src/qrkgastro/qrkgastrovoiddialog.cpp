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
#include "qrkgastrovoiddialog.h"
#include "ui_qrkgastrovoiddialog.h"

QRKGastroVoidDialog::QRKGastroVoidDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QRKGastroVoidDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->falseTalliedRadioButton->setChecked(true);
    m_selected = QRKGastro::CANCEL_FALSETALLIED;

    ui->buttonGroup->setId(ui->falseTalliedRadioButton, QRKGastro::CANCEL_FALSETALLIED);
    ui->buttonGroup->setId(ui->waitToLongRadioButton, QRKGastro::CANCEL_WAITTOLONG);
    ui->buttonGroup->setId(ui->wareSpoiledRadioButton, QRKGastro::CANCEL_WARESPOILED);
    ui->buttonGroup->setId(ui->cancellationRadioButton, QRKGastro::CANCEL_CANCELLATION);
    ui->buttonGroup->setId(ui->otherGuestsWishRadioButton, QRKGastro::CANCEL_OTHERGUESTWISH);

    connect(ui->okPushButton, &QrkPushButton::clicked, this, &QRKGastroVoidDialog::ok);
    connect(ui->buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &QRKGastroVoidDialog::finished);
}

QRKGastroVoidDialog::~QRKGastroVoidDialog()
{
    delete ui;
}

void QRKGastroVoidDialog::ok(bool)
{
    done(m_selected);
}

void QRKGastroVoidDialog::finished(int result)
{
    m_selected = result;
}
