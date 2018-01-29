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

#ifndef ROLESADMIN_H
#define ROLESADMIN_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"
#include "ui_base_admin.h"

#include <QWidget>
#include <QAbstractButton>
#include <QMap>

class QStringListModel;
class QGridLayout;
class QButtonGroup;
class QMenu;

class CKVSOFT_EXPORT RolesAdmin : public QWidget, private Ui::base_admin
{
        Q_OBJECT

    public:
        explicit RolesAdmin(QWidget *parent = 0);
        ~RolesAdmin();
        void maybeSaved();

    signals:
        void exitButtonClicked(bool);

    private slots:
        void modifyRole();
        void addRole();
        void deleteRole();

    private:
        QStringListModel *m_model;
        QAction *m_modifyAction;
        QAction *m_addAction;
        QAction *m_removeAction;
        QMenu *m_menu;
        QGridLayout *m_rolesgridLayout;
        QMap<QString, QMap<QString, QVariant> > m_roleperms;

        void contextMenu();
        void customContextMenuRequested(const QPoint &pos);
        void currentChanged(QModelIndex current, QModelIndex previous);
        void saveRole();

        void textChanged();
        void rolePermissions(QModelIndex idx);
        void manageRolePermissions();

        void buttonToggled(QAbstractButton *button, bool checked);


};

#endif // ROLESADMIN_H
