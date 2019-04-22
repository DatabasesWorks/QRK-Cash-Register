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

#include "userlogin.h"
#include "preferences/qrksettings.h"

#include <QApplication>

UserLogin::UserLogin(QWidget* parent)
    : base_login(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    QrkSettings settings;
    settings.beginGroup("ckvsoft");
    bool saveUserName = settings.value("savelastUserName", false).toBool();
    QString userName = settings.value("lastUserName", "").toString();
    settings.endGroup();

    if (saveUserName) {
        userLineEdit->setText(userName);
        passwordLineEdit->setFocus();
    } else {
        userLineEdit->setFocus();
    }

    savePasswordCheckBox->setChecked(saveUserName);

    setWindowTitle(tr("Benutzer Anmeldung"));
    windowIconLabel->setText("");
    windowNameLabel->setText(tr("Benutzer Anmeldung"));
    windowCommentLabel->setText(qApp->applicationName());
    programVersionLabel->setText(tr("Program Version:"));
    programVersionNoLabel->setText(qApp->applicationVersion());
    loginLabel->setText(tr("Benutzername"));
    passwordLabel->setText(tr("Kennwort"));
    passwordLineEdit->setText("");
    savePasswordCheckBox->setText("");
    label->setText(tr("Benutzername merken"));
    okPushButton->setText(tr("&Anmelden"));
    cancelPushButton->setText(tr("&Beenden"));

    connect(cancelPushButton, &QPushButton::clicked, this, &UserLogin::OnQuit);
    connect(okPushButton, &QPushButton::clicked, this, &UserLogin::OnLogin);
}

UserLogin::~UserLogin()
{
}

void UserLogin::reject()
{
    OnQuit();
}

void UserLogin::OnQuit()
{
    this->close();
    parentWidget()->close();
}

void UserLogin::OnLogin()
{
    QString username = userLineEdit->text();
    SecureByteArray password = passwordLineEdit->text().toUtf8();

    Crypto crypto;
    QString cryptPassword = crypto.encrypt(password);
    QString cryptPassword2 = RBAC::Instance()->getPasswordByUserName(username);

//    cryptPassword2 = cryptPassword;
    // Checking if username or password is empty
    if (username.isEmpty() || password.isEmpty())
        QMessageBox::warning(this, tr("Information!"), tr("Benutzername oder Kennwort darf nicht leer sein"));
    else if (cryptPassword2.isEmpty() || cryptPassword.compare(cryptPassword2) != 0)
        QMessageBox::critical(this, tr("Information!"), tr("Benutzername oder Kennwort falsch."));
    else {
        QrkSettings settings;
        settings.beginGroup("ckvsoft");
        if (savePasswordCheckBox->isChecked()) {
            settings.save2Settings("savelastUserName", true, false);
            settings.save2Settings("lastUserName", userLineEdit->text(), false);
        } else {
            settings.removeSettings("savelastUserName", false);
            settings.removeSettings("lastUserName", false);
        }
        settings.endGroup();

        RBAC::Instance()->setuserId(RBAC::Instance()->getUserIdByName(username));
        QDialog::accept();
        QDialog::close();
    }
}
