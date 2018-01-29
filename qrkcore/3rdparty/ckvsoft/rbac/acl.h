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

#ifndef ACL_H
#define ACL_H

#include "singleton/Singleton.h"
#include "3rdparty/ckvsoft/globals_ckvsoft.h"

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <QTimer>

class User;

class CKVSOFT_EXPORT Acl : public QObject
{
        Q_OBJECT

    public:

        enum Format {
            USER = 0,
            FULL
        };

        Acl(QObject *parent = 0);
        ~Acl();

        void setuserId(int userId);
        void settempUserId(int userId);
        void resetTempUserId();

        int getUserId();
        bool userHasRole(int id);
        bool hasPermission(QString permKey, bool allowtempuser = false);
        int getUserIdByName(QString name);
        QString getUsername();
        QString getUsername(int id);
        QString getDisplayname();
        QString getDisplayname(int id);
        int getGender();
        int getGender(int id);
        QString getAvatar();
        QString getAvatar(int id);

        QStringList getAllUsers();
        QStringList getUserRoles(int id, bool byId = false);
        QMap<QString, QMap<QString, QVariant> > getAllPerms();
        QStringList getAllRoles();
        int getRoleIdByName(QString &name);
        QString getPermNameFromID(int id);
        QString getPermKeyFromID(int id);
        int getPermIDfromKey(QString key);
        QString getPasswordByUserName(QString &name);
        QString getPasswordByUserId(int id);

        QMap<QString, QMap<QString, QVariant> > getRolePerms(int roleid);
        QMap<QString, QMap<QString, QVariant> > getUserPerms(int id, Format format);

        void saveRole(QString &name, int &id, QMap<QString, QMap<QString, QVariant> > &rolePerms);
        void deleteRole(QString &name, int id);

        void saveUser(User *user, int &id);
        void deleteUser(QString &name, int id);

        void savePerms(QMap<QString, QMap<QString, QVariant> > &permissions);

        bool Login();
        bool isMasterAdmin(int id);

    signals:
        void userChanged(int userId);
        void userChanged();

    private:
        void buildAcl();
        bool isMasterAdmin();

        QDateTime m_timeout;
        QTimer *m_timer;
        QMap<QString, QMap<QString, QVariant> > m_perms;                //Array : Stores the permissions for the user
        QMap<QString, QMap<QString, QVariant> > m_roleperms;

        int m_userId = -1;                  //Integer : Stores the ID of the current user
        int m_tempuserId = 0;              //Integer : Stores the ID of the temporary user
        bool m_masteradmin = false;         // Integer: Masteradmin have all rights
        QStringList m_userRoles;            //Array : Stores the roles of the current user
        QStringList getUserRoles();
        QMap<QString, QMap<QString, QVariant> > getUserPerms(int id);

        void merge(QMap<QString, QMap<QString, QVariant> > &map1, QMap<QString, QMap<QString, QVariant> > map2);
        QMap<QString, QMap<QString, QVariant> > getRolePerms(QStringList roleid);
        QString getRoleNameFromID(int id);
};

//Global variable
typedef Singleton<Acl> RBAC;

#endif // ACL_H
