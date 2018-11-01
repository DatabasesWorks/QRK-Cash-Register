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

#include "permissionsadmin.h"
#include "acl.h"

#include <QStringListModel>
#include <QKeyEvent>
#include <QMessageBox>

PermissionsAdmin::PermissionsAdmin(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    connect(saveButton, &QPushButton::clicked, this, &PermissionsAdmin::savePerms);
    connect(exitButton, &QPushButton::clicked, this, &PermissionsAdmin::exitButtonClicked);

    splitter->setSizes(QList<int>() << 100 << 200);
    splitter->setHandleWidth(5);
    infoLabel->setVisible(false);
    leftViewLabel->setText("");
    nameLabel->setText(tr("<b>Berechtigungen</b>"));
    iconLabel2->setPixmap(QPixmap(":src/icons/permission.png").scaled(32, 32, Qt::IgnoreAspectRatio, Qt::FastTransformation));

    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    nameEdit->setVisible(false);
    saveButton->setEnabled(false);
    groupBox->setTitle(tr("Berechtigungs Administration"));
    label->setText(tr("<b>Info:</b> Die Berechtigungen sind vorgegeben. Es kann nur der Anzeigename geändert werden."));
    splitter->setSizes(QList<int>() << 0 << 100);
    splitter->setHandleWidth(0);

    emit permissions();
}

PermissionsAdmin::~PermissionsAdmin()
{
}

void PermissionsAdmin::permissions()
{
    saveButton->setEnabled(false);
    QMap<QString, QMap<QString, QVariant> > perms = RBAC::Instance()->getAllPerms();

    QWidget *widget = new QWidget(this);
    QGridLayout *permsgridLayout = new QGridLayout(widget);

    QMapIterator<QString, QMap<QString, QVariant> > i(perms);

    permsgridLayout->addWidget(new QLabel(tr("<b>Berechtigung</b>")), 0, 0, 1, 1);
    permsgridLayout->addWidget(new QLabel(tr("<b>Anzeigename</b>")), 0, 1, 1, 1);

    int row = 1;
    while (i.hasNext()) {
        i.next();
        QMap<QString, QVariant> value = i.value();
        QString currentId = value.value("ID").toString();
        QString currentKey = value.value("permKey").toString();
        QString currentName = value.value("permName").toString();

        QLabel *permsLabel = new QLabel(widget);
        permsLabel->setText(currentKey);
        permsgridLayout->addWidget(permsLabel, row, 0, 1, 1);

        QLineEdit *permsEdit = new QLineEdit(widget);
        permsEdit->setText(currentName);
        permsEdit->setObjectName(QString("%1 %2 %3").arg(currentId).arg(currentKey).arg(currentName));
        permsgridLayout->addWidget(permsEdit, row, 1, 1, 1);

        permsLabel->setBuddy(permsEdit);

        connect(permsEdit, &QLineEdit::editingFinished, this, &PermissionsAdmin::displayNameChanged);
        connect(permsEdit, &QLineEdit::returnPressed, this, &PermissionsAdmin::returnPressed);

        qApp->processEvents();
        row++;
    }
    permsgridLayout->setAlignment(Qt::AlignTop);
    widget->setLayout(permsgridLayout);
    scrollArea->setWidget(widget);
}

void PermissionsAdmin::displayNameChanged()
{
    QLineEdit *edit = qobject_cast<QLineEdit *>(QObject::sender());
    QStringList list = edit->objectName().section(' ', 0, 1).split(' ');

    if (list.count() != 2) return;
    QString id = list[0];
    QString key = list[1];
    QString current = edit->objectName().section(' ', 2);
    QString name = edit->text();
    if (current == name) {
        if (m_permissions.size() == 0)
            saveButton->setEnabled(false);
        return;
    }

    QMap<QString, QVariant> perm;

    perm.insert("ID", id);
    perm.insert("permKey", key);
    perm.insert("permName", name);

    m_permissions.insert(id, perm);
    saveButton->setEnabled(true);
}

void PermissionsAdmin::savePerms()
{
    if (!m_permissions.isEmpty()) {
        RBAC::Instance()->savePerms(m_permissions);
        m_permissions.clear();
        saveButton->setEnabled(false);
        emit permissions();
    }
}

void PermissionsAdmin::returnPressed()
{
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);

    QApplication::postEvent(QObject::sender(), event);
}

void PermissionsAdmin::maybeSaved()
{
    if (m_permissions.count() != 0) {
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
            savePerms();
        } else {
            m_permissions.clear();
        }
    }
}
