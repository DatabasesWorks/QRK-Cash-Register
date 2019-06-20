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

#ifndef PERMISSIONSADMIN_H
#define PERMISSIONSADMIN_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"
#include "ui_base_admin.h"

#include <QWidget>
#include <QModelIndex>
#include <QMap>

class QStringListModel;

class CKVSOFT_EXPORT PermissionsAdmin : public QWidget, private Ui::base_admin
{
        Q_OBJECT

    public:
        explicit PermissionsAdmin(QWidget *parent = Q_NULLPTR);
        ~PermissionsAdmin();
        void maybeSaved();

    signals:
        void exitButtonClicked(bool);

    private:
        void permissions();
        void displayNameChanged();
        void returnPressed();
        void savePerms();

        QMap<QString, QMap<QString, QVariant> > m_permissions;

};

#endif // PERMISSIONSADMIN_H
