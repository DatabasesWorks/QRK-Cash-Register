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

#include "aclmanager.h"
#include "acl.h"
#include "aclwizard.h"
#include "useradmin.h"
#include "rolesadmin.h"
#include "qrkpushbutton.h"
#include "permissionsadmin.h"

#include <QProxyStyle>
#include <QStyleOptionTab>
#include <QTabWidget>
#include <QBoxLayout>
#include <QApplication>
#include <QDesktopWidget>

class CustomTabStyle : public QProxyStyle
{
    public:
        QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                               const QSize &size, const QWidget *widget) const
        {
            QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);

            if (type == QStyle::CT_TabBarTab)
                s.transpose();
            return s;
        }

        void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
        {
            if (element == CE_TabBarTabLabel) {
                if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
                    QStyleOptionTab opt(*tab);
                    opt.shape = QTabBar::RoundedNorth;
                    QProxyStyle::drawControl(element, &opt, painter, widget);
                    return;
                }
            }
            QProxyStyle::drawControl(element, option, painter, widget);
        }
};

AclManager::AclManager(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint)
{
    m_tabWidget = new QTabWidget(this);
    m_rolesadmin = new RolesAdmin(m_tabWidget);
    m_useradmin = new UserAdmin(m_tabWidget);
    m_permissionsadmin = new PermissionsAdmin(m_tabWidget);

    // FIXME: Apple do not Display Text with CustomTabStyle and QTabWidget::West
#ifndef __APPLE__
    m_tabWidget->setTabPosition(QTabWidget::West);
    m_tabWidget->tabBar()->setStyle(new CustomTabStyle);
#endif
    m_tabWidget->tabBar()->setIconSize(QSize(32, 32));
    m_tabWidget->addTab(m_useradmin, QPixmap(":src/icons/user.png"), tr("Benutzer"));
    m_tabWidget->addTab(m_rolesadmin, QPixmap(":src/icons/role.png"), tr("Rollen"));
    m_tabWidget->addTab(m_permissionsadmin, QPixmap(":src/icons/permission.png"), tr("Berechtigungen"));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_tabWidget, 1);
    mainLayout->addSpacing(12);
    setLayout(mainLayout);

    setWindowTitle(tr("Benutzer Manager"));
    resize(820, 500);
    setMinimumWidth(750);
    setMinimumHeight(400);

    if (QApplication::desktop()->height() < 650) {
        setMinimumWidth(700);
        setFixedHeight(550);
    }

    connect(m_useradmin, &UserAdmin::exitButtonClicked, m_useradmin, &UserAdmin::maybeSaved);
    connect(m_useradmin, &UserAdmin::exitButtonClicked, this, &AclManager::closeAclManager);
    connect(m_useradmin, &UserAdmin::closeAclManager, this, &AclManager::closeAclManager);
    connect(m_rolesadmin, &RolesAdmin::exitButtonClicked, m_rolesadmin, &RolesAdmin::maybeSaved);
    connect(m_rolesadmin, &RolesAdmin::exitButtonClicked, this, &AclManager::closeAclManager);
    connect(m_permissionsadmin, &PermissionsAdmin::exitButtonClicked, m_permissionsadmin, &PermissionsAdmin::maybeSaved);
    connect(m_permissionsadmin, &PermissionsAdmin::exitButtonClicked, this, &AclManager::closeAclManager);

    connect(m_useradmin, &UserAdmin::createRoles, this, &AclManager::createRoles);
    connect(m_tabWidget, &QTabWidget::currentChanged, m_useradmin, &UserAdmin::maybeSaved);
    connect(m_tabWidget, &QTabWidget::currentChanged, m_rolesadmin, &RolesAdmin::maybeSaved);
    connect(m_tabWidget, &QTabWidget::currentChanged, m_permissionsadmin, &PermissionsAdmin::maybeSaved);
}

AclManager::~AclManager()
{
}

void AclManager::closeAclManager()
{
    close();
}

void AclManager::createRoles()
{
    m_tabWidget->setCurrentWidget(m_rolesadmin);
}
