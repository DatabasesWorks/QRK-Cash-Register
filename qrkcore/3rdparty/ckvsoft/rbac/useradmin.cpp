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

#include "useradmin.h"
#include "aclwizard.h"
#include "acl.h"
#include "user.h"
#include "resetpassword.h"

#include <QDialog>
#include <QStringListModel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMenu>
#include <QTimer>
#include <QComboBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QRadioButton>
#include <QFileDialog>
#include <QKeyEvent>
#include <QDebug>

UserAdmin::UserAdmin(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);
    m_model = new QStringListModel(RBAC::Instance()->getAllUsers());
    ListView->setModel(m_model);

    connect(saveButton, &QPushButton::clicked, this, &UserAdmin::saveUser);
    connect(exitButton, &QPushButton::clicked, this, &UserAdmin::exitButtonClicked);

    splitter->setSizes(QList<int>() << 100 << 200);
    splitter->setHandleWidth(5);
    infoLabel->setVisible(false);
    leftViewLabel->setText(tr("<b>Benutzerliste</b>"));
    iconLabel->setPixmap(QPixmap(":src/icons/user.png").scaled(32, 32, Qt::IgnoreAspectRatio, Qt::FastTransformation));
    nameLabel->setText(tr("<b>Benutzername:</b" ">"));
    saveButton->setEnabled(false);
    groupBox->setTitle(tr("Benutzer Administration"));
    label->setText(tr("<b>Info:</b> Verwenden Sie in der <b>Benutzerliste</b> die <b>rechte Maustaste</b>, um das Kontextmenü auszuwählen."));

    m_userprofile.clear();
    contextMenu();

    ListView->selectionModel()->setCurrentIndex(m_model->index(0, 0), QItemSelectionModel::Select);
    emit userPermissions(m_model->index(0, 0));
}

UserAdmin::~UserAdmin()
{
    delete m_model;
    qDeleteAll(m_userprofile.begin(), m_userprofile.end());
}

void UserAdmin::contextMenu()
{
    ListView->setToolTip(tr("Verwenden Sie die rechte Maustaste, um das Kontextmenü auszuwählen"));

    m_menu = new QMenu(ListView);
    m_modifyUserAction = m_menu->addAction(tr("Benutzer bearbeiten"), this, SLOT(userProfile())); // new syntax not working in qt5.6
    ListView->addAction(m_modifyUserAction);

    m_modifyRolesAction = m_menu->addAction(tr("Benutzerrollen bearbeiten"), this, SLOT(modifyUserRoles())); // new syntax not working in qt5.6
    ListView->addAction(m_modifyRolesAction);

    m_modifyPermsAction = m_menu->addAction(tr("Benutzerberechtigung bearbeiten"), this, SLOT(modifyUserPerms()));
    ListView->addAction(m_modifyPermsAction);

    m_addAction = m_menu->addAction(tr("Neuer Benutzer"), this, SLOT(addUser()));
    ListView->addAction(m_addAction);

    m_removeAction = m_menu->addAction(tr("Benutzer löschen"), this, SLOT(deleteUser()));
    ListView->addAction(m_removeAction);

    ListView->setContextMenuPolicy(Qt::CustomContextMenu);
    ListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ListView, &QListView::customContextMenuRequested, this, &UserAdmin::customContextMenuRequested);
    connect(ListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &UserAdmin::currentChanged);
}

void UserAdmin::customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);

    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    bool masterAdmin = RBAC::Instance()->isMasterAdmin(RBAC::Instance()->getUserIdByName(itemText));

    m_modifyUserAction->setEnabled(ListView->selectionModel()->selectedRows().size() == 1 && RBAC::Instance()->hasPermission("admin_edit_user"));
    m_modifyRolesAction->setEnabled(ListView->selectionModel()->selectedRows().size() == 1 && !masterAdmin  && RBAC::Instance()->hasPermission("admin_edit_userroles"));
    m_modifyPermsAction->setEnabled(ListView->selectionModel()->selectedRows().size() == 1 && !masterAdmin  && RBAC::Instance()->hasPermission("admin_edit_userperms"));
    m_addAction->setEnabled(ListView->selectionModel()->selectedRows().size() < 2  && RBAC::Instance()->hasPermission("admin_create_user"));
    m_removeAction->setEnabled(ListView->selectionModel()->selectedRows().size() > 0 && RBAC::Instance()->hasPermission("admin_delete_user"));
    m_menu->exec(QCursor::pos());
}

void UserAdmin::modifyUserRoles()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();

    ListView->setCurrentIndex(idx);
    nameEdit->setText(itemText);
    //    saveButton->setEnabled(false);
    emit manageUserRoles();
    nameEdit->setFocus();
}

void UserAdmin::modifyUserPerms()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();

    ListView->setCurrentIndex(idx);
    nameEdit->setText(itemText);

    emit manageUserPermissions();
    nameEdit->setFocus();
}

void UserAdmin::saveUser()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getUserIdByName(itemText);
    saveUserById(id);
    m_model->setStringList(RBAC::Instance()->getAllUsers());
    ListView->setCurrentIndex(idx);
    saveButton->setEnabled(false);
    emit userPermissions(idx);
}

bool UserAdmin::saveUserById(int id)
{
    if (m_userprofile.contains(id)) {
        m_user = m_userprofile.take(id);
        RBAC::Instance()->saveUser(m_user, id);
        delete m_user;
        m_user = 0;
        return true;
    }
    return true;
}

void UserAdmin::deleteUser()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getUserIdByName(itemText);
    int ownId = RBAC::Instance()->getUserId();

    QMessageBox msgBox;

    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (RBAC::Instance()->getAllUsers().count() != 1 && RBAC::Instance()->isMasterAdmin(id)) {
        msgBox.setText(tr("Benutzer löschen? Sind versuchen den \"Ultimativen Administrator\" %1 zu löschen.\nDieser Benutzer kann nur als letzer bestehender Benutzer gelöscht werden.").arg(itemText));
        msgBox.setButtonText(QMessageBox::No, tr("OK"));
    } else {
        if (id == ownId)
            msgBox.setText(tr("ACHTUNG!\nSie sind im Begriff sich selbst zu löschen! Sind Sie sicher das Sie das möchten?").arg(itemText));
        else
            msgBox.setText(tr("Benutzer löschen? Sind Sie sicher das der Benutzer '%1' gelöscht werden soll?").arg(itemText));
        msgBox.setStandardButtons(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Ja"));
        msgBox.setButtonText(QMessageBox::No, tr("Nein"));
    }

    if (msgBox.exec() == QMessageBox::Yes) {
        RBAC::Instance()->deleteUser(itemText, id);
        m_model->setStringList(RBAC::Instance()->getAllUsers());

        ListView->setCurrentIndex(m_model->index(0, 0));
        saveButton->setEnabled(false);
        nameEdit->clear();
        if (RBAC::Instance()->isMasterAdmin(id)) {
            RBAC::Instance()->setuserId(0);
        } else if (id == ownId) {
            RBAC::Instance()->setuserId(-1);
            emit closeAclManager();
        }
    }

    emit userPermissions(m_model->index(0, 0));
}

void UserAdmin::addUser()
{
    QString userName = "";

    if (RBAC::Instance()->getAllRoles().isEmpty() && !RBAC::Instance()->getAllUsers().isEmpty()) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Neuer Benutzer"));
        msgBox.setText(tr("Es sind keine Benutzerrollen verfügbar.\nUm einen Benutzer anlegen zu können muss zumindest eine Benutzerrolle vorhanden sein."));
        msgBox.exec();
        AclWizard wizard(AclWizard::USERANDROLES);
        wizard.exec();
        if (wizard.getRoleName().isEmpty())
            return;

        userName = wizard.getUserName();
        emit newRoleAdded();
    } else {
        AclWizard wizard(AclWizard::USER);
        wizard.exec();
        userName = wizard.getUserName();
    }
    if (!userName.isEmpty()) {
        m_model->insertRow(m_model->rowCount());
        QModelIndex idx = m_model->index(m_model->rowCount() - 1);
        m_model->setData(idx, userName);
        ListView->setCurrentIndex(idx);
        nameEdit->setFocus();
        QTimer::singleShot(0, nameEdit, &QLineEdit::selectAll);
        emit userPermissions(idx);
    }
}

void UserAdmin::userPermissions(QModelIndex idx)
{
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getUserIdByName(itemText);

    bool masterAdmin = RBAC::Instance()->isMasterAdmin(id);

    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else {
        m_user =  new User(id, this);
        m_userprofile.insert(id, m_user);
    }

    bool enabled = m_user->getChanged();
    saveButton->setEnabled(enabled);

    QMap<QString, QMap<QString, QVariant> > userperms = RBAC::Instance()->getUserPerms(id, Acl::FULL);
    QStringList userroles = RBAC::Instance()->getUserRoles(id);

    nameEdit->setText(itemText);

    QWidget *widget = new QWidget(this);
    QGridLayout *usergridLayout = new QGridLayout(widget);

    QListIterator<QString> r(userroles);
    QMapIterator<QString, QMap<QString, QVariant> > p(userperms);

    int row = 0;
    if (id == 0) {
        usergridLayout->addWidget(new QLabel(tr("Keine Benutzer vorhanden")), 0, 1, 1, 4, Qt::AlignCenter);
        row++;
    }

    infoLabel->setVisible(false);

    usergridLayout->addWidget(new QLabel(tr("<b>Benutzerrollen</b>")), row, 0, 1, 1);

    if (!r.hasNext() && !masterAdmin)
        usergridLayout->addWidget(new QLabel(tr("Keine Benutzerrollen")), row, 1, 1, 4, Qt::AlignCenter);
    else if (masterAdmin)
        usergridLayout->addWidget(new QLabel("[Chuck Norris]"), row, 1, 1, 4, Qt::AlignCenter);
    row++;

    while (r.hasNext() && !masterAdmin) {
        usergridLayout->addWidget(new QLabel(r.next()), row, 0, 1, 1);;
        row++;
    }

    usergridLayout->addWidget(new QLabel(tr("<b>Benutzerrechte</b>")), row, 0, 1, 1);
    if (!p.hasNext() && !masterAdmin) {
        usergridLayout->addWidget(new QLabel(tr("Keine Benutzerrechte")), row, 1, 1, 4, Qt::AlignCenter);
    } else if (masterAdmin) {
        usergridLayout->addWidget(new QLabel(tr("alle")), row, 2, 1, 1, Qt::AlignRight);
        QLabel *allowLabel = new QLabel();
        allowLabel->setPixmap(QPixmap(":src/icons/ok.png").scaled(16, 16));
        usergridLayout->addWidget(allowLabel, row, 3, 1, 1, Qt::AlignLeft);
    }
    row++;

    while (p.hasNext() && !masterAdmin) {
        p.next();

        QMap<QString, QVariant> value = p.value();
        QString current = value.value("name").toString();

        QLabel *userLabel = new QLabel(widget);
        userLabel->setWordWrap(true);
        userLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        userLabel->setText(current);
        usergridLayout->addWidget(userLabel, row, 0, 1, 1);

        bool allow = value.value("value").toBool();
        if (allow) {
            QLabel *allowLabel = new QLabel();
            allowLabel->setPixmap(QPixmap(":src/icons/ok.png").scaled(16, 16));
            usergridLayout->addWidget(allowLabel, row, 1, 1, 1, Qt::AlignLeft);
        } else {
            QLabel *denyLabel = new QLabel();
            denyLabel->setPixmap(QPixmap(":src/icons/cancel.png").scaled(16, 16));
            usergridLayout->addWidget(denyLabel, row, 1, 1, 1, Qt::AlignLeft);
        }

        QLabel *inheritted = new QLabel((value.value("inheritted").toBool()) ? tr("(geerbt)") : tr("explizit"), widget);
        usergridLayout->addWidget(inheritted, row, 2, 1, 1, Qt::AlignLeft);

        qApp->processEvents();
        row++;
    }

    usergridLayout->setAlignment(Qt::AlignTop);
    widget->setLayout(usergridLayout);
    scrollArea->setWidget(widget);
}

void UserAdmin::manageUserPermissions()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getUserIdByName(itemText);

    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else {
        m_user = new User(id, this);
        m_userprofile.insert(id, m_user);
        m_user->setPermissionsMap(RBAC::Instance()->getUserPerms(id, Acl::FULL));
    }

    saveButton->setEnabled(m_user->getChanged());

    QMap<QString, QMap<QString, QVariant> > userperms = m_user->getPermissions();
    QMap<QString, QMap<QString, QVariant> > allPerms = RBAC::Instance()->getAllPerms();

    QWidget *widget = new QWidget(this);
    QGridLayout *usergridLayout = new QGridLayout(widget);

    QMapIterator<QString, QMap<QString, QVariant> > i(allPerms);
    if (!i.hasNext())
        usergridLayout->addWidget(new QLabel(tr("Keine Benutzerberechtigung")), 1, 1, 1, 4, Qt::AlignCenter);

    usergridLayout->addWidget(new QLabel(tr("<b>Benutzerberechtigung</b>")), 0, 0, 1, 1);

    int row = 1;
    while (i.hasNext()) {
        i.next();

        QMap<QString, QVariant> rPerm;
        QMap<QString, QVariant> value = i.value();

        QLabel *permLabel = new QLabel(widget);
        permLabel->setWordWrap(true);
        permLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        permLabel->setText(tr("%1").arg(value.value("permName").toString()));
        usergridLayout->addWidget(permLabel, row, 0, 1, 1);

        QString key = value.value("permKey").toString();

        if (userperms.contains(key))
            rPerm = userperms.value(key);

        QComboBox *combo = new QComboBox(widget);
        combo->setObjectName(key);
        combo->addItem(tr("erlauben"), "allow");
        combo->addItem(tr("verweigern"), "deny");
        if (rPerm.value("value").toBool() == true && rPerm.value("inheritted").toBool() != true)
            combo->setCurrentIndex(combo->findData("allow"));
        if (rPerm.value("value").toBool() == false && rPerm.value("inheritted").toBool() != true)
            combo->setCurrentIndex(combo->findData("deny"));

        QString iv;
        bool select = false;
        if (rPerm.value("inheritted").toBool() == true || !userperms.contains(key)) {
            select = true;
            if (rPerm.value("value").toBool() == true)
                iv = tr("(erlauben)");
            else
                iv = tr("(verweigern)");
        }
        combo->addItem(tr("geerbt %1").arg(iv), "inheritted");
        if (select)
            combo->setCurrentIndex(combo->findData("inheritted"));

        usergridLayout->addWidget(combo, row, 1, 1, 1);

        qApp->processEvents();
        connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &UserAdmin::comboIndexChanged);

        row++;
    }

    usergridLayout->setAlignment(Qt::AlignTop);
    widget->setLayout(usergridLayout);
    scrollArea->setWidget(widget);
}

void UserAdmin::comboIndexChanged(int idx)
{
    QComboBox *combo = qobject_cast<QComboBox *>(QObject::sender());
    QString key = combo->objectName();

    QModelIndex index = ListView->currentIndex();
    QString itemText = index.data(Qt::DisplayRole).toString();

    if (itemText.isEmpty()) return;
    int userid = RBAC::Instance()->getUserIdByName(itemText);
    if (userid < 1) return;

    if(m_userprofile.contains(userid)) {
        m_user = m_userprofile.value(userid);
    } else {
        m_user = new User(userid, this);
        m_userprofile.insert(userid, m_user);
    }

    int permid = RBAC::Instance()->getPermIDfromKey(key);
    QString data = combo->itemData(idx).toString();

    QMap<QString, QVariant> map;

    map.insert("ID", permid);
    map.insert("permID", permid);
    map.insert("value", (data == "allow"));
    map.insert("inheritted", (data == "inheritted"));
    m_user->insertPermissionsMap(key, map);
    m_user->setChanged(true);
    saveButton->setEnabled(true);
}

void UserAdmin::manageUserRoles()
{
    QModelIndex idx = ListView->currentIndex();
    QString itemText = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getUserIdByName(itemText);

    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else {
        m_user = new User(id, this);
        m_userprofile.insert(id, m_user);
    }

    saveButton->setEnabled(m_user->getChanged());

    QStringList userroles = RBAC::Instance()->getUserRoles(id);
    QMapIterator<QString, bool> r(m_user->getRoles());
    while(r.hasNext()) {
        r.next();
        if (!r.value() && userroles.contains(r.key()))
            userroles.removeAll(r.key());
        else if (r.value() && !userroles.contains(r.key()))
            userroles.append(r.key());
    }
    QStringList allroles = RBAC::Instance()->getAllRoles();

    QWidget *widget = new QWidget(this);
    QGridLayout *usergridLayout = new QGridLayout(widget);

    QListIterator<QString> i(allroles);
    if (allroles.size() == 0) {
        QPushButton *createRoles = new QPushButton(widget);
        createRoles->setText(tr("erstellen"));
        connect(createRoles, &QPushButton::clicked, this, &UserAdmin::createRoles);
        usergridLayout->addWidget(new QLabel(tr("Keine Rolle vorhanden.")), 0, 0, 1, 3, Qt::AlignCenter);
        usergridLayout->addWidget(createRoles, 0, 3, 1, 1, Qt::AlignCenter);
    }

    int row = 0;
    while (i.hasNext()) {
        QString txt = i.next();
        QCheckBox *box = new QCheckBox(widget);
        box->setObjectName(txt);
        box->setLayoutDirection(Qt::RightToLeft);
        box->setText(txt);
        if (userroles.contains(txt))
            box->setChecked(true);

        usergridLayout->addWidget(box, row, 0, 1, 1);

        qApp->processEvents();
        connect(box, &QCheckBox::stateChanged, this, &UserAdmin::checkStateChanged);

        row++;
    }

    usergridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    widget->setLayout(usergridLayout);
    scrollArea->setWidget(widget);
}

void UserAdmin::checkStateChanged(int state)
{
    Q_UNUSED(state);

    QModelIndex idx = ListView->currentIndex();
    QString username = idx.data(Qt::DisplayRole).toString();
    int id = RBAC::Instance()->getUserIdByName(username);

    if(m_userprofile.contains(id)) {
        m_user = m_userprofile.value(id);
    } else {
        m_user = new User(id, this);
        m_userprofile.insert(id, m_user);
    }

    QCheckBox *check = qobject_cast<QCheckBox *>(QObject::sender());
    QString name = check->objectName();
    m_user->insertRoleMap(name, check->isChecked());
    m_user->setChanged(true);
}

void UserAdmin::userProfile()
{

    QModelIndex idx = ListView->currentIndex();
    QString username = idx.data(Qt::DisplayRole).toString();

    if (username.isEmpty()) {
        emit addUser();
        return;
    }

    int id = RBAC::Instance()->getUserIdByName(username);
    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else
        m_user = new User(id, this);

    saveButton->setEnabled(m_user->getChanged());

    QWidget *widget = new QWidget(this);

    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    QGridLayout *usergridLayout = new QGridLayout;

    QLabel *userProfileIconLabel = new QLabel(widget);
    if (m_user->getAvatar().isEmpty())
        if (m_user->getGender() == User::MALE)
            userProfileIconLabel->setPixmap(QPixmap(":src/icons/user_male_480.png").scaled(100, 100,Qt::KeepAspectRatio));
        else
            userProfileIconLabel->setPixmap(QPixmap(":src/icons/user_female_480.png").scaled(100, 100,Qt::KeepAspectRatio));
    else
        userProfileIconLabel->setPixmap(QPixmap(m_user->getAvatar()).scaled(100, 100,Qt::KeepAspectRatio));

    usergridLayout->addWidget(userProfileIconLabel, 0, 0, 3, 1);

    QLabel *userNameLabel = new QLabel(tr("Benutzername:"),widget);
    userNameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    usergridLayout->addWidget(userNameLabel, 0, 1, 1, 1);
    QLineEdit *userNameLineEdit = new QLineEdit(m_user->getUserName(), widget);
    usergridLayout->addWidget(userNameLineEdit, 0, 2, 1, 2);

    QLabel *displayNameLabel = new QLabel(tr("Anzeigename:"), widget);
    displayNameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    usergridLayout->addWidget(displayNameLabel, 1, 1, 1, 1);
    QLineEdit *displayNameLineEdit = new QLineEdit(m_user->getDisplayName(), widget);
    usergridLayout->addWidget(displayNameLineEdit, 1, 2, 1, 2);

    QLabel *genderLabel = new QLabel(tr("Geschlecht:"), widget);
    genderLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    usergridLayout->addWidget(genderLabel, 2, 1, 1, 1);

    QRadioButton *maleRadioButton = new QRadioButton(tr("männlich"), widget);
    QButtonGroup *genderButtonGroup = new QButtonGroup(widget);
    genderButtonGroup->addButton(maleRadioButton, (int)User::MALE);
    usergridLayout->addWidget(maleRadioButton, 2, 2, 1, 1);

    QRadioButton *femaleRadioButton = new QRadioButton(tr("weiblich"), widget);
    genderButtonGroup->addButton(femaleRadioButton, (int)User::FEMALE);
    usergridLayout->addWidget(femaleRadioButton, 2, 3, 1, 1);

    if (m_user->getGender() == User::MALE)
        maleRadioButton->setChecked(true);
    else
        femaleRadioButton->setChecked(true);

    QPushButton *avatarButton = new QPushButton(tr("Avatar"), widget);
    QPushButton *passwordButton = new QPushButton(tr("Kennwort"), widget);

    connect(passwordButton, &QPushButton::clicked, this, &UserAdmin::changePassword);
    connect(avatarButton, &QPushButton::clicked, this, &UserAdmin::avatarButtonClicked);
    connect(genderButtonGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &UserAdmin::genderButtomClicked);
    connect(displayNameLineEdit, &QLineEdit::textChanged, this, &UserAdmin::changeDisplayName);
    connect(userNameLineEdit, &QLineEdit::textChanged, this, &UserAdmin::changeUserName);

    usergridLayout->addWidget(avatarButton, 3, 0, 1, 1);
    usergridLayout->addWidget(passwordButton, 4, 0, 1, 1);

    QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    usergridLayout->addItem(horizontalSpacer, 4, 1, 1, 3);

    userNameLabel->setBuddy(userNameLineEdit);
    displayNameLabel->setBuddy(displayNameLineEdit);

    usergridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    vBoxLayout->addLayout(usergridLayout);
    QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    vBoxLayout->addItem(verticalSpacer);
    widget->setLayout(vBoxLayout);
    scrollArea->setWidget(widget);
}

void UserAdmin::avatarButtonClicked()
{

    QString filename = QFileDialog::getOpenFileName(this, tr("Avatar laden ..."), "", tr("JPEG (*.jpg *.jpeg);;PNG (*.png)"), 0, QFileDialog::DontUseNativeDialog);
    if (filename.isNull())
        return;

    QModelIndex idx = ListView->currentIndex();
    QString username = idx.data(Qt::DisplayRole).toString();

    if (username.isEmpty()) {
        return;
    }

    int id = RBAC::Instance()->getUserIdByName(username);

    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else
        m_user = new User(id, this);

    m_user->setAvatar(filename);
    m_userprofile.insert(id, m_user);
    m_user->setChanged(true);

    emit userProfile();

}

void UserAdmin::changeUserName(const QString &username)
{
    QModelIndex idx = ListView->currentIndex();
    QString currentname = idx.data(Qt::DisplayRole).toString();

    if (currentname.isEmpty()) {
        return;
    }

    int id = RBAC::Instance()->getUserIdByName(currentname);

    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else
        m_user = new User(id, this);

    m_user->setUserName(username);
    m_userprofile.insert(id, m_user);
    m_user->setChanged(true);

    saveButton->setEnabled(true);
}

void UserAdmin::changeDisplayName(const QString &displayname)
{
    QModelIndex idx = ListView->currentIndex();
    QString currentname = idx.data(Qt::DisplayRole).toString();

    if (currentname.isEmpty()) {
        return;
    }

    int id = RBAC::Instance()->getUserIdByName(currentname);

    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else
        m_user = new User(id, this);

    m_user->setDisplayName(displayname);
    m_userprofile.insert(id, m_user);
    m_user->setChanged(true);

    saveButton->setEnabled(true);
}

void UserAdmin::changePassword()
{
    QModelIndex idx = ListView->currentIndex();
    QString username = idx.data(Qt::DisplayRole).toString();

    if (username.isEmpty()) {
        return;
    }

    int id = RBAC::Instance()->getUserIdByName(username);

    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else
        m_user = new User(id, this);

    ResetPassword resetPW(m_user->getUserId(), this);
    if (resetPW.exec() == QDialog::Accepted) {
        QString pw1, pw2;
        resetPW.getPassword(pw1, pw2);
        m_user->setNewPassword(pw1, pw2);
        m_user->setChanged(true);
    }
    emit userProfile();
}

void UserAdmin::genderButtomClicked(int buttonId)
{
    QModelIndex idx = ListView->currentIndex();
    QString username = idx.data(Qt::DisplayRole).toString();

    if (username.isEmpty()) {
        return;
    }

    int id = RBAC::Instance()->getUserIdByName(username);

    if (m_userprofile.contains(id))
        m_user = m_userprofile.value(id);
    else
        m_user = new User(id, this);

    m_user->setGender((User::GENDER)buttonId);
    m_user->setChanged(true);
    m_userprofile.insert(id, m_user);

    emit userProfile();
}

void UserAdmin::currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);
    maybeSaved();
    emit userPermissions(current);
}

void UserAdmin::maybeSaved()
{
    if (m_userprofile.count() != 0) {
        if (!m_userprofile.begin().value()->getChanged()) {
            m_user = m_userprofile.take(m_userprofile.begin().key());
            delete m_user;
            m_user = 0;
            return;
        }
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
            QMapIterator<int, User*> i(m_userprofile);
            while(i.hasNext()) {
                i.next();
                saveUserById(i.key());
            }
        } else {
            m_user = m_userprofile.take(m_userprofile.begin().key());
            delete m_user;
            m_user = 0;
        }
    }
}

void UserAdmin::returnPressed()
{
    QKeyEvent *event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);

    QApplication::postEvent(QObject::sender(), event);
}
