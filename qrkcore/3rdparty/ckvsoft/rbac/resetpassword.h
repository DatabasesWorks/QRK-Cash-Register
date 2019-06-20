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

#ifndef RESETPASSWORD_H
#define RESETPASSWORD_H

#include "base_login.h"

class CKVSOFT_EXPORT ResetPassword : public base_login
{
        Q_OBJECT

    public:
        explicit ResetPassword(int userId, QWidget *parent = Q_NULLPTR);
        ~ResetPassword();
        void getPassword(QString &pw1, QString &pw2);

    private slots:
        void OnChange();

    private:

        int m_userid;

        SecureByteArray m_password1;
        SecureByteArray m_password2;
};

#endif // RESETPASSWORD_H
