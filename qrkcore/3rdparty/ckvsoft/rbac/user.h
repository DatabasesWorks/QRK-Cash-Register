/*
 * This file is part of QRK - Qt Registrier Kasse
 *
 * Copyright (C) 2015-2018 Christian Kvasny <chris@ckvsoft.at>
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

#ifndef USER_H
#define USER_H

#include "crypto.h"

#include <QObject>
#include <QMap>
#include <QVariant>

class User : public QObject
{
        Q_OBJECT

    public:
        enum GENDER {
            MALE = 0,
            FEMALE
        };

        explicit User(int userId, QObject *parent = nullptr);
        int getUserId();
        QString getUserName();
        QString getDisplayName();
        GENDER getGender();
        QString getAvatar();
        SecureByteArray getPassword();
        QMap<QString, QMap<QString, QVariant> > getPermissions();
        QMap<QString, bool> getRoles();

        void setUserName(QString username);
        void setDisplayName(QString displayname);
        void setGender(GENDER gender);
        void setAvatar(QString avatar);
        void setPassword();
        void setPassword(QString password);
        void setNewPassword(QString password1, QString password2);
        void getNewPassword(QString &password1, QString &password2);
        bool checkNewPassword();
        void insertPermissionsMap(QString key, QMap<QString, QVariant> value);
        void setPermissionsMap(QMap<QString, QMap<QString, QVariant> > permissions);
        void insertRoleMap(QString key, bool value);
        void setRoleMap(QMap<QString, bool> roles);
        bool getChanged();
        void setChanged(bool changed);

    signals:

    public slots:
    private:
        int m_userId;
        QString m_username;
        QString m_displayname;
        GENDER m_gender;
        QString m_avatar;
        SecureByteArray m_password;
        SecureByteArray m_password1;
        SecureByteArray m_password2;
        QMap<QString, QMap<QString, QVariant> > m_permissions;
        QMap<QString, bool> m_roles;
        bool m_changed;

};

#endif // USER_H
