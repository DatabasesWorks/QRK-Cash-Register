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

#ifndef ACLMANAGER_H
#define ACLMANAGER_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"

#include <QDialog>

class UserAdmin;
class RolesAdmin;
class PermissionsAdmin;
class QTabWidget;

class CKVSOFT_EXPORT Widget : public QWidget
{
        Q_OBJECT
    public:
        explicit Widget(QWidget *parent = Q_NULLPTR) : QWidget(parent)
        {
        }

        void disableWidgets()
        {
            QList<QWidget *> widgets = this->findChildren<QWidget *>();

            foreach(QWidget * widget, widgets){
                widget->setEnabled(false);
            }
        }
};

class CKVSOFT_EXPORT AclManager : public QDialog
{
        Q_OBJECT

    public:
        explicit AclManager(QWidget *parent = Q_NULLPTR);
        ~AclManager();

        void createFirstRoleAndUser();

    private:
        UserAdmin *m_useradmin;
        RolesAdmin *m_rolesadmin;
        PermissionsAdmin *m_permissionsadmin;
        QTabWidget *m_tabWidget;

        void createRoles();
        void closeAclManager();
};

#endif // ACLMANAGER_H
