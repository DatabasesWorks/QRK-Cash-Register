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

#ifndef BASE_LOGIN_H
#define BASE_LOGIN_H

#include "qrkpushbutton.h"
#include "3rdparty/ckvsoft/globals_ckvsoft.h"
#include "acl.h"
#include "crypto.h"

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>

class CKVSOFT_EXPORT base_login : public QDialog
{
        Q_OBJECT

    public:
        explicit base_login(QWidget *parent = Q_NULLPTR);
        ~base_login();

    protected:
        QLineEdit *userLineEdit;
        QLineEdit *passwordLineEdit;
        QrkPushButton *okPushButton;
        QrkPushButton *cancelPushButton;
        QCheckBox *savePasswordCheckBox;

        QLabel *windowIconLabel;
        QLabel *windowNameLabel;
        QLabel *windowCommentLabel;
        QLabel *programVersionLabel;
        QLabel *programVersionNoLabel;
        QLabel *loginLabel;
        QLabel *passwordLabel;
        QLabel *label;
        QLabel *logoLabel;
};

#endif // BASE_LOGIN_H
