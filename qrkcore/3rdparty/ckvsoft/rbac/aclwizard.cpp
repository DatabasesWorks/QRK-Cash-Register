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

#include "aclwizard.h"
#include "flowlayout.h"
#include "acl.h"
#include "user.h"

#include <QtWidgets>
#include <QRadioButton>
#include <QButtonGroup>

AclWizard::AclWizard(WizardType type, QWidget *parent)
    : QWizard(parent), m_type(type)
{
    bool isFirstUser = RBAC::Instance()->getAllUsers().isEmpty();

    if (isFirstUser) m_type = USER;

    switch (m_type) {
    case USERANDROLES:
        addPage(new AclIntroPage);
        addPage(new AclRoleInfoPage);
        addPage(new AclUserInfoPage);
        addPage(new AclUserRolesInfoPage);
        setWindowTitle(tr("Benutzerrolle und Benutzer anlegen ..."));
        break;
    case ROLE:
        addPage(new AclRoleIntroPage);
        addPage(new AclRoleInfoPage);
        setWindowTitle(tr("Benutzerrolle anlegen ..."));
        break;
    case USER:
        addPage(new AclUserIntroPage);
        addPage(new AclUserInfoPage);
        if (!isFirstUser)
            addPage(new AclUserRolesInfoPage);
        setWindowTitle(tr("Benutzer anlegen ..."));
        break;
    }

    resize(QSize(600, 400).expandedTo(minimumSizeHint()));
}

void AclWizard::accept()
{
    if (m_type == ROLE || m_type == USERANDROLES)
        m_roleName = field("roleName").toByteArray();
    if (m_type == USER || m_type == USERANDROLES)
        m_userName = field("userName").toByteArray();

    QDialog::accept();
}

void AclWizard::reject()
{
    if (m_type == ROLE || m_type == USERANDROLES)
        m_roleName = field("roleName").toByteArray();
    if (m_type == USER || m_type == USERANDROLES)
        m_userName = field("userName").toByteArray();

    QDialog::reject();
}

QByteArray AclWizard::getRoleName()
{
    return field("roleName").toByteArray();
}

QByteArray AclWizard::getUserName()
{
    return m_userName;
}

AclIntroPage::AclIntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Benutzerrolle und Benutzer anlegen"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":src/icons/user.png").scaled(200, 200,Qt::KeepAspectRatio));

    label = new QLabel(tr("Mit diesem Assistenten können Sie Benutzer anlegen und Rollen anlegen bzw. vergeben. Wenn Sie keinen Benutzer anlegen können Sie die QRK Registrierkasse wie gewohnt verwenden. Wenn Sie mit Benutzern arbeiten wollen, sollten Sie zumindest einen Administrator anlegen, der im Ernstfall auch die Passwörter der Benutzer zurücksetzen kann."));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);
}

AclRoleIntroPage::AclRoleIntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Benutzerrolle anlegen"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":src/icons/role.png").scaled(200, 200,Qt::KeepAspectRatio));

    label = new QLabel(tr("Mit diesem Assistenten können Sie eine neue Rolle anlegen. Wenn Sie keine Rolle anlegen können keine Benutzer angelegt werden. Wenn Sie mit Benutzern arbeiten wollen, sollten Sie zumindest eine Administrator Rolle anlegen."));
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);

}

AclRoleInfoPage::AclRoleInfoPage(QWidget *parent)
    : QWizardPage(parent)
{
    m_scrollArea = new QScrollArea(this);
    setTitle(tr("Benutzerrollen Information"));
    setSubTitle(tr("Vergeben Sie der Benutzerrolle Berechtigungen."));
    setPixmap(QWizard::LogoPixmap, QPixmap(":src/icons/role.png").scaled(80, 80, Qt::IgnoreAspectRatio, Qt::FastTransformation));

    m_roleNameExistsLabel = new QLabel("");
    m_roleNameExistsLabel->setStyleSheet("QLabel { color : red; }");

    QLabel *infoLabel = new QLabel(tr("Berechtigungen:\nerl = erlauben, ver = verweigern, ign = ignorieren"));

    m_roleNameLabel = new QLabel(tr("Rollenname:"));
    m_roleNameLabel->setAlignment(Qt::AlignRight);
    m_roleNameLineEdit = new QLineEdit;
    m_roleNameLabel->setBuddy(m_roleNameLineEdit);

    registerField("roleName", m_roleNameLineEdit);

    connect(m_roleNameLineEdit, &QLineEdit::textChanged, this, &AclRoleInfoPage::completeChanged);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_roleNameExistsLabel, 0, 0, 1, 2);
    layout->addWidget(m_roleNameLabel, 1, 0);
    layout->addWidget(m_roleNameLineEdit, 1, 1);
    layout->addWidget(infoLabel, 2, 0, 1, 2);
    layout->addWidget(m_scrollArea, 3, 0, 1, 2);

    setLayout(layout);

    emit manageRolePermissions();
}

bool AclRoleInfoPage::isComplete() const
{
    return !m_roleperms.isEmpty() && !m_roleNameLineEdit->text().isEmpty();
}

bool AclRoleInfoPage::validatePage()
{
    QString name = m_roleNameLineEdit->text();
    int id = RBAC::Instance()->getRoleIdByName(name);

    m_roleId = property("roleId").toInt();
    if (id > 0 && id != m_roleId) {
        m_roleNameExistsLabel->setText(tr("Der Rollenname '%1' ist schon in Verwendung. Bitte wählen Sie einen neuen.").arg(name));
        m_roleNameLineEdit->clear();
        return false;
    }

    RBAC::Instance()->saveRole(name, id, m_roleperms);
    setProperty("roleId", id);
    return true;
}

void AclRoleInfoPage::initializePage()
{
    QString name = field("roleName").toByteArray();

    m_roleId = RBAC::Instance()->getRoleIdByName(name);
    setProperty("roleId", m_roleId);
}

void AclRoleInfoPage::manageRolePermissions()
{
    QString name = m_roleNameLineEdit->text();
    int id = RBAC::Instance()->getRoleIdByName(name);

    QMap<QString, QMap<QString, QVariant> > roleperms = RBAC::Instance()->getRolePerms(id);
    QMap<QString, QMap<QString, QVariant> > allPerms = RBAC::Instance()->getAllPerms();

    QWidget *widget = new QWidget(this);
    QGridLayout *rolesgridLayout = new QGridLayout(widget);

    QMapIterator<QString, QMap<QString, QVariant> > i(allPerms);
    if (!i.hasNext())
        rolesgridLayout->addWidget(new QLabel(tr("Keine Rollenberechtigung")), 1, 1, 1, 4, Qt::AlignCenter);

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
        connect(buttonGroupRoles, static_cast<void (QButtonGroup::*)(QAbstractButton *, bool)>(&QButtonGroup::buttonToggled), this, &AclRoleInfoPage::buttonToggled);
        row++;
    }

    rolesgridLayout->setAlignment(Qt::AlignTop);
    widget->setLayout(rolesgridLayout);
    m_scrollArea->setWidget(widget);
}

void AclRoleInfoPage::buttonToggled(QAbstractButton *button, bool checked)
{
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
            if (list[0] == "ignore" && m_roleperms.contains(pK))
                map.insert("ignore", true);
            else if (list[0] == "allow")
                hP = true;
            map.insert("value", hP);
            map.insert("name", permName);
            map.insert("ID", id);
            m_roleperms.insert(pK, map);
            emit completeChanged();
        }
    }
}

AclUserIntroPage::AclUserIntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Einleitung"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":src/icons/user.png").scaled(200, 200,Qt::KeepAspectRatio));

    bool isFirstUser = RBAC::Instance()->getAllUsers().isEmpty();

    label = new QLabel(tr("Hier können Sie ein neues Benutzerkonto erstellen.\n"
                          "Wenn Sie das Kennwort erstellt haben, vergessen Sie es nicht – es gibt keine Möglichkeit zum Wiederherstellen eines verlorenen Kennworts.\n"));

    if (isFirstUser)
        label->setText(tr("%1\nSie sind dabei den ersten Benutzer zu erstellen, dieser wird automatisch zum \"Ultimativen Administrator\" [Chuck Norris] mit allen Rechten.").arg(label->text()));
    else
        label->setText(tr("%1\nUm aus dem neuen Konto ein Administratorkonto zu machen, vergeben Sie die ensprechende Administrator Rolle").arg(label->text()));

    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    setLayout(layout);

}

AclUserInfoPage::AclUserInfoPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Benutzer Information"));
    setSubTitle(tr("Geben Sie grundlegende Informationen zu der Person an, für die Sie das Benutzerkonto erstellen möchten."));
    setPixmap(QWizard::LogoPixmap, QPixmap(":src/icons/user.png").scaled(80, 80, Qt::IgnoreAspectRatio, Qt::FastTransformation));

    m_userNameLabel = new QLabel(tr("Benutzername:"));
    m_userNameLabel->setAlignment(Qt::AlignRight);
    m_userNameLineEdit = new QLineEdit;
    m_userNameLabel->setBuddy(m_userNameLineEdit);

    m_displayNameLabel = new QLabel(tr("Anzeigename:"));
    m_displayNameLabel->setAlignment(Qt::AlignRight);
    m_displayNameLineEdit = new QLineEdit;
    m_displayNameLabel->setBuddy(m_displayNameLineEdit);

    m_passwordLabel = new QLabel(tr("Kennwort:"));
    m_passwordLabel->setAlignment(Qt::AlignRight);
    m_passwordLineEdit = new QLineEdit;
    m_passwordLineEdit->setEchoMode(QLineEdit::Password);
    m_passwordLabel->setBuddy(m_passwordLineEdit);

    m_password2Label = new QLabel(tr("Kennwort wiederholen:"));
    m_password2Label->setAlignment(Qt::AlignRight);
    m_password2LineEdit = new QLineEdit;
    m_password2LineEdit->setEchoMode(QLineEdit::Password);
    m_password2Label->setBuddy(m_password2LineEdit);

    m_maleRadioButton = new QRadioButton(tr("männlich"));
    m_maleRadioButton->setChecked(true);
    m_femaleRadioButton = new QRadioButton(tr("weiblich"));
    m_buttongroup = new QButtonGroup(this);
    m_buttongroup->addButton(m_maleRadioButton, 0);
    m_buttongroup->addButton(m_femaleRadioButton, 1);

    m_avatarLineEdit = new QLineEdit;
    m_avatarLineEdit->setPlaceholderText(tr("Pfad zu einen Benutzer Avatar (optional)"));

    registerField("userName", m_userNameLineEdit);
    registerField("displayName", m_displayNameLineEdit);
    registerField("password", m_passwordLineEdit);
    registerField("avatar", m_avatarLineEdit);
    registerField("male", m_maleRadioButton);
    registerField("female", m_femaleRadioButton);

//    registerField("buttongroup", m_buttongroup);

    QFrame *lineH1 = new QFrame;
    lineH1->setFrameShape(QFrame::HLine);
    QFrame *lineH2 = new QFrame;
    lineH2->setFrameShape(QFrame::HLine);

    bool isFirstUser = RBAC::Instance()->getAllUsers().isEmpty();

    QLabel *infoLabel = new QLabel;
    if (isFirstUser)
        infoLabel->setText(tr("Sie sind dabei den \"Ultimativen Administrator\" zu erstellen. Merken Sie sich das Kennwort und/oder verwahren es an einem sicheren Ort - es gibt keine Möglichkeit zum Wiederherstellen eines verlorenen Kennworts."));
    else
        infoLabel->setText(tr("Merken Sie sich das Kennwort und/oder verwahren es an einem sicheren Ort - es gibt keine Möglichkeit zum Wiederherstellen eines verlorenen Kennworts."));

    infoLabel->setWordWrap(true);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(m_userNameLabel, 0, 0);
    layout->addWidget(m_userNameLineEdit, 0, 1);
    layout->addWidget(m_displayNameLabel, 0, 2);
    layout->addWidget(m_displayNameLineEdit, 0, 3);
    layout->addWidget(m_passwordLabel, 2, 0);
    layout->addWidget(m_passwordLineEdit, 2, 1);
    layout->addWidget(m_password2Label, 2, 2);
    layout->addWidget(m_password2LineEdit, 2, 3);
    layout->addWidget(lineH1, 3, 0, 1, 4);
    layout->addWidget(new QLabel(tr("Geschlecht:")), 4, 0);
    layout->addWidget(m_maleRadioButton, 4, 1);
    layout->addWidget(m_femaleRadioButton, 4, 2);
    layout->addWidget(new QLabel(tr("Avatar:")), 5, 0);
    layout->addWidget(m_avatarLineEdit, 5, 1, 1, 2);

    layout->addWidget(lineH2, 6, 0, 1, 4);
    layout->addWidget(infoLabel, 7, 0, 1, 4);
    setLayout(layout);

    connect(m_userNameLineEdit, &QLineEdit::editingFinished, this, &AclUserInfoPage::nameFinished);
    connect(m_userNameLineEdit, &QLineEdit::textChanged, this, &AclUserInfoPage::nameTextChanged);
    connect(m_displayNameLineEdit, &QLineEdit::editingFinished, this, &AclUserInfoPage::displayNameFinished);
    connect(m_passwordLineEdit, &QLineEdit::editingFinished, this, &AclUserInfoPage::passwordFinished);
    connect(m_password2LineEdit, &QLineEdit::textChanged, this, &AclUserInfoPage::password2TextChanged);
    connect(m_password2LineEdit, &QLineEdit::editingFinished, this, &AclUserInfoPage::password2Finished);
}

void AclUserInfoPage::nameTextChanged()
{
    QString name = m_userNameLineEdit->text();
    QString displayname = m_displayNameLineEdit->text();

    if (name.left(name.length() - 1) == displayname || displayname.left(displayname.length() - 1) == name)
        m_displayNameLineEdit->setText(name);

    emit completeChanged();
}

void AclUserInfoPage::nameFinished()
{
    emit completeChanged();
}

void AclUserInfoPage::displayNameFinished()
{
    emit completeChanged();
}

void AclUserInfoPage::passwordFinished()
{
    emit completeChanged();
}

void AclUserInfoPage::password2Finished()
{
    emit completeChanged();
}

void AclUserInfoPage::password2TextChanged()
{
    if (m_passwordLineEdit->text() == m_password2LineEdit->text())
        emit completeChanged();
}

bool AclUserInfoPage::isComplete() const
{
    return
            !m_userNameLineEdit->text().isEmpty() &&
            !m_passwordLineEdit->text().isEmpty() &&
            !m_password2LineEdit->text().isEmpty() &&
            !m_displayNameLineEdit->text().isEmpty();
}

bool AclUserInfoPage::validatePage()
{
    if (m_passwordLineEdit->text().compare(m_password2LineEdit->text()) != 0) {
        QMessageBox::warning(this, tr("Warnung!"), tr("Kennwort Eingabe wiederholen. Die beiden Eingabefelder sind ungleich."));
        m_password2LineEdit->setText("");
        m_passwordLineEdit->setFocus();
        m_passwordLineEdit->selectAll();
        return false;
    }

    if (RBAC::Instance()->getAllUsers().isEmpty()) {
        int id = -1;
        User user(id);
        user.setUserName(m_userNameLineEdit->text());
        user.setDisplayName(m_displayNameLineEdit->text());
        user.setPassword(m_passwordLineEdit->text());
        User::GENDER gender = m_maleRadioButton->isChecked()?User::MALE:User::FEMALE;
        user.setGender(gender);
        user.setAvatar(m_avatarLineEdit->text());
        RBAC::Instance()->saveUser(&user, id);
        RBAC::Instance()->setuserId(RBAC::Instance()->getUserIdByName(user.getUserName()));
    }

    return isComplete();
}

AclUserRolesInfoPage::AclUserRolesInfoPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Rollen Information"));
    setSubTitle(tr("Fügen Sie Rollen zu der Person hinzu, für die Sie das Benutzerkonto erstellen möchten.\nWeitere Rollen können im Manager angelegt werde."));
    setPixmap(QWizard::LogoPixmap, QPixmap(":src/icons/role.png").scaled(200, 200,Qt::KeepAspectRatio));
}

void AclUserRolesInfoPage::manageUserRoles()
{
    QStringList allroles = RBAC::Instance()->getAllRoles();

    m_widget = new QWidget(this);
    //    QGridLayout *usergridLayout = new QGridLayout(m_widget);
    FlowLayout *userFlowLayout = new FlowLayout(m_widget);

    QListIterator<QString> i(allroles);
    int row = 0;
    while (i.hasNext()) {
        QString txt = i.next();
        QCheckBox *box = new QCheckBox(m_widget);
        box->setObjectName(txt);
        box->setLayoutDirection(Qt::RightToLeft);
        box->setText(txt);

        //        usergridLayout->addWidget(box, row, 0, 1, 1);
        userFlowLayout->addWidget(box);

        qApp->processEvents();
        connect(box, &QCheckBox::stateChanged, this, &AclUserRolesInfoPage::checkStateChanged);

        row++;
    }

    //    usergridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    //    m_widget->setLayout(usergridLayout);
    m_widget->setLayout(userFlowLayout);
    m_widget->show();
}

void AclUserRolesInfoPage::checkStateChanged(int state)
{
    Q_UNUSED(state);
    QCheckBox *check = qobject_cast<QCheckBox *>(QObject::sender());
    QString name = check->objectName();
    if (check->isChecked())
        m_userroles.insert(name, true);
    else
        m_userroles.remove(name);

    emit completeChanged();
}

void AclUserRolesInfoPage::initializePage()
{
    manageUserRoles();
    m_username = field("userName").toString();
    m_displayname = field("displayName").toString();
    m_password = field("password").toString();
    m_avatar = field("avatar").toString();
    m_male = field("male").toBool();
    m_id = RBAC::Instance()->getUserIdByName(m_username);
}

bool AclUserRolesInfoPage::isComplete() const
{
    return !m_userroles.isEmpty() &&
            !m_username.isEmpty() &&
            !m_password.isEmpty() &&
            !m_displayname.isEmpty();
}

bool AclUserRolesInfoPage::validatePage()
{
    User user(m_id);
    user.setUserName(m_username);
    user.setDisplayName(m_displayname);
    user.setPassword(m_password);
    User::GENDER gender = m_male?User::MALE:User::FEMALE;
    user.setGender(gender);
    user.setAvatar(m_avatar);
    user.setRoleMap(m_userroles);
    RBAC::Instance()->saveUser(&user, m_id);
    return true;
}

void AclWizard::createFirstRoleAndUser()
{
    bool roles = RBAC::Instance()->getAllRoles().isEmpty();
    bool users = RBAC::Instance()->getAllUsers().isEmpty();

    if (roles && users) {
        AclWizard wizard(AclWizard::USERANDROLES);
        wizard.exec();
    } else if (roles) {
        AclWizard wizard(AclWizard::ROLE);
        wizard.exec();
    } else if (users) {
        AclWizard wizard(AclWizard::USER);
        wizard.exec();
    }
}
