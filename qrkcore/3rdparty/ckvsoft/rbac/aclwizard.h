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

#ifndef ACLWIZARD_H
#define ACLWIZARD_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"

#include <QWizard>
#include <QMap>
#include <QVariant>

class QLabel;
class QLineEdit;
class QScrollArea;
class QButtonGroup;
class QRadioButton;

class CKVSOFT_EXPORT AclWizard : public QWizard
{
        Q_OBJECT

    public:
        enum WizardType {
            USERANDROLES = 0,
            USER,
            ROLE
        };

        AclWizard(WizardType type, QWidget *parent = 0);

        void accept() override;
        void reject() override;

        QByteArray getRoleName();
        QByteArray getUserName();

        static void createFirstRoleAndUser();

    private:
        QByteArray m_roleName = "";
        QByteArray m_userName = "";
        WizardType m_type;
};

class CKVSOFT_EXPORT AclIntroPage : public QWizardPage
{
        Q_OBJECT

    public:
        AclIntroPage(QWidget *parent = 0);

    private:
        QLabel *label;
};

class CKVSOFT_EXPORT AclRoleIntroPage : public QWizardPage
{
        Q_OBJECT

    public:
        AclRoleIntroPage(QWidget *parent = 0);

    private:
        QLabel *label;
};

class CKVSOFT_EXPORT AclRoleInfoPage : public QWizardPage
{
        Q_OBJECT

    public:
        AclRoleInfoPage(QWidget *parent = 0);

    private:
        bool isComplete() const;
        bool validatePage();
        void initializePage();

        void manageRolePermissions();
        void buttonToggled(QAbstractButton *button, bool checked);
        QMap<QString, QMap<QString, QVariant> > m_roleperms;
        QLabel *m_roleNameExistsLabel;
        QLabel *m_roleNameLabel;
        QLineEdit *m_roleNameLineEdit;
        QScrollArea *m_scrollArea;
        int m_roleId;
};

class CKVSOFT_EXPORT AclUserIntroPage : public QWizardPage
{
        Q_OBJECT

    public:
        AclUserIntroPage(QWidget *parent = 0);

    private:
        QLabel *label;
};

class CKVSOFT_EXPORT AclUserInfoPage : public QWizardPage
{
        Q_OBJECT

    public:
        AclUserInfoPage(QWidget *parent = 0);

    private:
        bool isComplete() const;
        bool validatePage();

        void nameFinished();
        void nameTextChanged();
        void displayNameFinished();
        void passwordFinished();
        void password2Finished();
        void password2TextChanged();

        QLabel *m_userNameLabel;
        QLabel *m_passwordLabel;
        QLabel *m_password2Label;
        QLabel *m_displayNameLabel;
        QLineEdit *m_userNameLineEdit;
        QLineEdit *m_passwordLineEdit;
        QLineEdit *m_password2LineEdit;
        QLineEdit *m_displayNameLineEdit;
        QLineEdit *m_avatarLineEdit;

        QButtonGroup *m_buttongroup;
        QRadioButton *m_maleRadioButton;
        QRadioButton *m_femaleRadioButton;
};

class CKVSOFT_EXPORT AclUserRolesInfoPage : public QWizardPage
{
        Q_OBJECT

    public:
        AclUserRolesInfoPage(QWidget *parent = 0);

    private:
        void manageUserRoles();
        void checkStateChanged(int state);

        void initializePage();
        bool isComplete() const;
        bool validatePage();

        QWidget *m_widget;
        QScrollArea *m_scrollArea;
        QMap<QString, bool> m_userroles;
        QString m_username;
        QString m_password;
        QString m_displayname;
        QString m_avatar;
        bool m_male;
        int m_id;
};

#endif
