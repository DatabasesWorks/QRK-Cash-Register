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

#include "tempuserlogin.h"
#include "preferences/qrksettings.h"

#include <QApplication>

TempUserLogin::TempUserLogin(QWidget* parent)
    : base_login(parent)
{

    resize(500, 380);
    setAttribute(Qt::WA_DeleteOnClose);
    userLineEdit->setFocus();
    savePasswordCheckBox->setVisible(false);

    setWindowTitle(tr("Temporäre Benutzer Anmeldung"));
    windowIconLabel->setText("");
    windowNameLabel->setText(tr("Temporäre Benutzer Anmeldung"));
    windowCommentLabel->setWordWrap(true);
    windowCommentLabel->setText(tr("Zugriff verweigert. Sie können durch Angabe eines Benutzers dessen Rechte erhalten."));
    programVersionLabel->setText(tr("Die durch diesen Benutzer erhaltenen Rechte werden nach 120 Sekunden wieder entfernt."));
    programVersionLabel->setMaximumSize(QSize(16777215, 16777215));
    programVersionLabel->setWordWrap(true);
    programVersionNoLabel->setText("");
    loginLabel->setText(tr("Benutzername"));
    passwordLabel->setText(tr("Kennwort"));
    passwordLineEdit->setText("");
    savePasswordCheckBox->setText("");
    okPushButton->setText(tr("&Ok"));
    cancelPushButton->setText(tr("&Abbrechen"));

    connect(cancelPushButton, &QPushButton::clicked, this, &TempUserLogin::reject);
    connect(okPushButton, &QPushButton::clicked, this, &TempUserLogin::OnLogin);
}

TempUserLogin::~TempUserLogin()
{
}

void TempUserLogin::OnLogin()
{
    QString username = userLineEdit->text();
    SecureByteArray password = passwordLineEdit->text().toUtf8();

    Crypto crypto;
    QString cryptPassword = crypto.encrypt(password);
    QString cryptPassword2 = RBAC::Instance()->getPasswordByUserName(username);

    // Checking if username or password is empty
    if (username.isEmpty() || password.isEmpty())
        QMessageBox::warning(this, tr("Information!"), tr("Benutzername oder Kennwort darf nicht leer sein"));
    else if (cryptPassword2.isEmpty() || cryptPassword.compare(cryptPassword2) != 0)
        QMessageBox::critical(this, tr("Information!"), tr("Benutzername oder Kennwort falsch."));
    else {
        RBAC::Instance()->settempUserId(RBAC::Instance()->getUserIdByName(username));
        QDialog::accept();
        QDialog::close();
    }
}
