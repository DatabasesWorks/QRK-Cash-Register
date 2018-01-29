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

#include "rolesadmin.h"
#include "aclwizard.h"
#include "acl.h"

#include <QObject>
#include <QStringListModel>
#include <QRadioButton>
#include <QGridLayout>
#include <QButtonGroup>
#include <QMenu>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>

RolesAdmin::RolesAdmin(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);
    m_model = new QStringListModel(RBAC::Instance()->getAllRoles());
    ListView->setModel(m_model);

    connect(saveButton, &QPushButton::clicked, this, &RolesAdmin::saveRole);
    connect(nameEdit, &QLineEdit::textChanged, this, &RolesAdmin::textChanged);
    connect(nameEdit, &QLineEdit::editingFinished, this, &RolesAdmin::textChanged);
    connect(ListView, &QListView::activated, this, &RolesAdmin::rolePermissions);

    connect(exitButton, &QPushButton::clicked, this, &RolesAdmin::exitButtonClicked);

    m_roleperms.clear();
    splitter->setSizes(QList<int>() << 100 << 200);
    splitter->setHandleWidth(5);

    infoLabel->setVisible(false);
    leftViewLabel->setText(tr("<b>Rollenliste</b>"));
    iconLabel->setPixmap(QPixmap(":src/icons/role.png").scaled(32, 32, Qt::IgnoreAspectRatio, Qt::FastTransformation));
    nameLabel->setText(tr("<b>Rollenname:</b>"));
    saveButton->setEnabled(false);
    groupBox->setTitle(tr("Rollen Administration"));
    label->setText(tr("<b>Info:</b> Verwenden Sie in der <b>Rollenliste</b> die <b>rechte Maustaste</b>, um das Kontextmenü auszuwählen."));

    contextMenu();

    ListView->selectionModel()->setCurrentIndex(m_model->index(0, 0), QItemSelectionModel::Select);
    connect(ListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &RolesAdmin::currentChanged);

    emit rolePermissions(m_model->index(0, 0));
}

RolesAdmin::~RolesAdmin()
{
    delete m_model;
}

void RolesAdmin::contextMenu()
{
    ListView->setToolTip(tr("Verwenden Sie die rechte Maustaste, um das Kontextmenü auszuwählen"));

    m_menu = new QMenu(ListView);
    m_modifyAction = m_menu->addAction(tr("Rollenberechtigung bearbeiten"), this, SLOT(modifyRole())); //new syntax not working in qt5.6
    ListView->addAction(m_modifyAction);

    m_addAction = m_menu->addAction(tr("Neue Rolle"), this, SLOT(addRole()));
    ListView->addAction(m_addAction);

    m_removeAction = m_menu->addAction(tr("Rolle löschen"), this, SLOT(deleteRole()));
    ListView->addAction(m_removeAction);

    ListView->setContextMenuPolicy(Qt::CustomContextMenu);
    ListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ListView, &QListView::customContextMenuRequested, this, &RolesAdmin::customContextMenuRequested);
}

void RolesAdmin::customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);

    m_modifyAction->setEnabled(ListView->selectionModel()->selectedRows().size() == 1 && RBAC::Instance()->hasPermission("admin_edit_role"));
    m_addAction->setEnabled(ListView->selectionModel()->selectedRows().size() < 2 && RBAC::Instance()->hasPermission("admin_create_role"));
    m_removeAction->setEnabled(ListView->selectionModel()->selectedRows().size() > 0 && RBAC::Instance()->hasPermission("admin_delete_role"));
    m_menu->exec(QCursor::pos());
}

void RolesAdmin::currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);
    maybeSaved();
    emit rolePermissions(current);
}

void RolesAdmin::modifyRole()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();

    ListView->setCurrentIndex(idx);
    nameEdit->setText(itemText);

    emit manageRolePermissions();
    nameEdit->setFocus();
}

void RolesAdmin::saveRole()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getRoleIdByName(itemText);

    QString name = nameEdit->text();

    RBAC::Instance()->saveRole(name, id, m_roleperms);
    m_roleperms.clear();
    m_model->setStringList(RBAC::Instance()->getAllRoles());
    ListView->setCurrentIndex(idx);
    saveButton->setEnabled(false);
    emit rolePermissions(idx);
}

void RolesAdmin::deleteRole()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getRoleIdByName(itemText);

    RBAC::Instance()->deleteRole(itemText, id);
    m_model->setStringList(RBAC::Instance()->getAllRoles());
    ListView->setCurrentIndex(m_model->index(0, 0));
    emit rolePermissions(m_model->index(0, 0));
}

void RolesAdmin::addRole()
{
    AclWizard wizard(AclWizard::ROLE);

    wizard.exec();
    if (!wizard.getRoleName().isEmpty()) {
        m_model->insertRow(m_model->rowCount());
        QModelIndex idx = m_model->index(m_model->rowCount() - 1);
        QString name = wizard.getRoleName();
        m_model->setData(idx, name);
        //        RBAC::Instance()->saveRole( User(name, -1, m_userperms, m_userroles);
        ListView->setCurrentIndex(idx);
        nameEdit->setFocus();
        QTimer::singleShot(0, nameEdit, &QLineEdit::selectAll);
        emit rolePermissions(idx);
    }
}

void RolesAdmin::textChanged()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    QString editText = nameEdit->text();

    bool contains = false;
    bool changed = (editText != itemText);

    if (changed) {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            contains = editText == m_model->index(i, 0).data(Qt::DisplayRole).toString();
            if (contains) {
                nameEdit->setStyleSheet("background: red");
                break;
            } else {
                nameEdit->setStyleSheet("");
            }
        }
    } else {
        nameEdit->setStyleSheet("");
    }

    saveButton->setEnabled(false);
    if (changed && !contains && !nameEdit->text().isEmpty())
        saveButton->setEnabled(true);
}

void RolesAdmin::rolePermissions(QModelIndex idx)
{
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getRoleIdByName(itemText);

    QMap<QString, QMap<QString, QVariant> > roleperms = RBAC::Instance()->getRolePerms(id);

    nameEdit->setText(itemText);

    QWidget *widget = new QWidget(this);
    QGridLayout *rolesgridLayout = new QGridLayout(widget);

    QMapIterator<QString, QMap<QString, QVariant> > i(roleperms);
    if (!i.hasNext())
        rolesgridLayout->addWidget(new QLabel(tr("Keine Rollenberechtigung")), 0, 1, 1, 4, Qt::AlignCenter);

    infoLabel->setVisible(false);
    rolesgridLayout->addWidget(new QLabel(tr("<b>Rollenberechtigung</b>")), 0, 0, 1, 1);

    int row = 1;
    while (i.hasNext()) {
        i.next();
        QMap<QString, QVariant> value = i.value();
        QString current = value.value("name").toString();

        QLabel *roleLabel = new QLabel(widget);
        roleLabel->setWordWrap(true);
        roleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        roleLabel->setText(tr("%1").arg(current));
        rolesgridLayout->addWidget(roleLabel, row, 0, 1, 1);

        bool allow = value.value("value").toBool();
        if (allow) {
            QLabel *allowLabel = new QLabel();
            allowLabel->setPixmap(QPixmap(":src/icons/ok.png").scaled(16, 16));
            rolesgridLayout->addWidget(allowLabel, row, 1, 1, 1, Qt::AlignLeft);
        } else {
            QLabel *denyLabel = new QLabel();
            denyLabel->setPixmap(QPixmap(":src/icons/cancel.png").scaled(16, 16));
            rolesgridLayout->addWidget(denyLabel, row, 1, 1, 1, Qt::AlignLeft);
        }

        qApp->processEvents();
        row++;
    }

    rolesgridLayout->setAlignment(Qt::AlignTop);
    widget->setLayout(rolesgridLayout);
    scrollArea->setWidget(widget);
}

void RolesAdmin::manageRolePermissions()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getRoleIdByName(itemText);

    QMap<QString, QMap<QString, QVariant> > roleperms = RBAC::Instance()->getRolePerms(id);
    QMap<QString, QMap<QString, QVariant> > allPerms = RBAC::Instance()->getAllPerms();

    QWidget *widget = new QWidget(this);
    QGridLayout *rolesgridLayout = new QGridLayout(widget);

    QMapIterator<QString, QMap<QString, QVariant> > i(allPerms);
    if (!i.hasNext())
        rolesgridLayout->addWidget(new QLabel(tr("Keine Rollenberechtigung")), 1, 1, 1, 4, Qt::AlignCenter);

    infoLabel->setVisible(true);

    rolesgridLayout->addWidget(new QLabel(tr("<b>Rollenberechtigung</b>")), 0, 0, 1, 1);
    rolesgridLayout->addWidget(new QLabel("<b>erl</b>"), 0, 1, 1, 1);
    rolesgridLayout->addWidget(new QLabel("<b>ver</b>"), 0, 2, 1, 1);
    rolesgridLayout->addWidget(new QLabel("<b>ign</b>"), 0, 3, 1, 1);

    int row = 1;
    while (i.hasNext()) {
        i.next();

        QMap<QString, QVariant> value = i.value();

        QButtonGroup *buttonGroupRoles;
        buttonGroupRoles = new QButtonGroup(widget);

        int id = value.value("ID").toInt();

        QLabel *roleLabel = new QLabel(widget);
        roleLabel->setWordWrap(true);
        roleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        roleLabel->setText(tr("%1").arg(value.value("permName").toString()));
        rolesgridLayout->addWidget(roleLabel, row, 0, 1, 1);

        QRadioButton *allowradioButton = new QRadioButton(widget);
        allowradioButton->setObjectName(QString("allow %1").arg(id));
        buttonGroupRoles->addButton(allowradioButton, 0);
        rolesgridLayout->addWidget(allowradioButton, row, 1, 1, 1);

        QRadioButton *denyradioButton = new QRadioButton(widget);
        denyradioButton->setObjectName(QString("deny %1").arg(id));
        buttonGroupRoles->addButton(denyradioButton, 1);
        rolesgridLayout->addWidget(denyradioButton, row, 2, 1, 1);

        QRadioButton *ignoreradioButton = new QRadioButton(widget);
        ignoreradioButton->setObjectName(QString("ignore %1").arg(id));
        buttonGroupRoles->addButton(ignoreradioButton, 2);
        rolesgridLayout->addWidget(ignoreradioButton, row, 3, 1, 1);

        if (roleperms.contains(i.key())) {
            QMap<QString, QVariant> roleValue = roleperms.value(i.key());
            if (roleValue.value("value").toBool())
                allowradioButton->setChecked(true);
            else
                denyradioButton->setChecked(true);
        } else {
            ignoreradioButton->setChecked(true);
        }

        qApp->processEvents();
        connect(buttonGroupRoles, static_cast<void (QButtonGroup::*)(QAbstractButton *, bool)>(&QButtonGroup::buttonToggled), this, &RolesAdmin::buttonToggled);
        row++;
    }

    rolesgridLayout->setAlignment(Qt::AlignTop);
    widget->setLayout(rolesgridLayout);
    scrollArea->setWidget(widget);
}

void RolesAdmin::buttonToggled(QAbstractButton *button, bool checked)
{
    saveButton->setEnabled(true);
    if (checked) {
        QStringList list = button->objectName().split(' ');
        if (list.count() == 2) {
            int id = list[1].toInt();
            QString permName = RBAC::Instance()->getPermNameFromID(id);
            bool hP = false;

            QString pK = RBAC::Instance()->getPermKeyFromID(id);
            QMap<QString, QVariant> map;
            map.insert("perm", pK);
            map.insert("inheritted", true);
            if (list[0] == "ignore") /* && m_roleperms.contains(pK)) */
                map.insert("ignore", true);
            else if (list[0] == "allow")
                hP = true;
            map.insert("value", hP);
            map.insert("name", permName);
            map.insert("ID", id);
            m_roleperms.insert(pK, map);
        }
    }
}

void RolesAdmin::maybeSaved()
{
    if (m_roleperms.count() != 0) {
        QMessageBox msgBox;

        msgBox.setIcon(QMessageBox::Warning);
        msgBox.addButton(QMessageBox::Save);
        msgBox.setDefaultButton(QMessageBox::Save);
        msgBox.addButton(QMessageBox::Cancel);

        msgBox.setText(tr("Es wurden nicht alle Änderungen gespeichert. Möchten Sie die änderungen speichern?"));
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        msgBox.setButtonText(QMessageBox::Save, tr("Speichern"));
        msgBox.setButtonText(QMessageBox::Cancel, tr("Verwerfen"));
        if (msgBox.exec() == QMessageBox::Save) {
            saveRole();
        } else {
            m_roleperms.clear();
        }
    }
}
