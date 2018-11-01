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

#ifndef USERADMIN_H
#define USERADMIN_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"
#include "ui_base_admin.h"
#include <QWidget>
#include <QModelIndex>
#include <QMap>

class QAbstractButton;
class QStringListModel;
class QMenu;
class User;

class CKVSOFT_EXPORT UserAdmin : public QWidget, private Ui::base_admin
{
        Q_OBJECT

    public:
        explicit UserAdmin(QWidget *parent = 0);
        ~UserAdmin();
        void maybeSaved();

    signals:
        void exitButtonClicked();
        void createRoles();
        void newRoleAdded();
        void closeAclManager();

    private slots:
        void userProfile();
        void modifyUserPerms();
        void modifyUserRoles();
        void deleteUser();
        void addUser();
        void avatarButtonClicked();
        void changePassword();
        void changeUserName(const QString &username);
        void changeDisplayName(const QString &displayname);
        void genderButtomClicked(int id);


    private:
        QStringListModel *m_model;
        QAction *m_modifyPermsAction;
        QAction *m_modifyRolesAction;
        QAction *m_modifyUserAction;
        QAction *m_addAction;
        QAction *m_removeAction;
        QMenu *m_menu;
        User *m_user = 0;

        QMap<int, User*> m_userprofile;

        void contextMenu();
        void customContextMenuRequested(const QPoint &pos);
        void saveUser();
        bool saveUserById(int id);

        void userPermissions(QModelIndex idx);
        void manageUserPermissions();
        void manageUserRoles();
        void comboIndexChanged(int idx);
        void checkStateChanged(int state);
        void currentChanged(QModelIndex current, QModelIndex previous);
        void returnPressed();


};

#endif // USERADMIN_H
