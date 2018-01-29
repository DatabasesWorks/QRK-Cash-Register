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

#include "user.h"
#include "acl.h"

#include <QFile>

User::User(int userid, QObject *parent) : QObject(parent), m_userId(userid), m_changed(false)
{
    m_username = RBAC::Instance()->getUsername(m_userId);
    m_displayname = RBAC::Instance()->getDisplayname(m_userId);
    m_avatar = RBAC::Instance()->getAvatar(m_userId);
    switch (RBAC::Instance()->getGender(m_userId))
    {
    case User::MALE:
        m_gender = User::MALE;
        break;
    case User::FEMALE:
        m_gender = User::FEMALE;
        break;
    default:
        m_gender = User::FEMALE;
    }
}

int User::getUserId()
{
    return m_userId;
}
QString User::getUserName()
{
    return m_username;
}

QString User::getDisplayName()
{
    return m_displayname;
}

User::GENDER User::getGender()
{
    return m_gender;
}

QString User::getAvatar()
{
    return m_avatar;
}

SecureByteArray User::getPassword()
{
    return m_password;
}

QMap<QString, QMap<QString, QVariant> > User::getPermissions()
{
    return m_permissions;
}

bool User::getChanged()
{
    return m_changed;
}

void User::setChanged(bool changed)
{
    m_changed = changed;
}

void User::setPermissionsMap(QMap<QString, QMap<QString, QVariant> > permissions)
{
    m_permissions = permissions;
}

void User::insertPermissionsMap(QString key, QMap<QString, QVariant> value)
{
    m_permissions.insert(key, value);
}

QMap<QString, bool> User::getRoles()
{
    return m_roles;
}

void User::insertRoleMap(QString key, bool value)
{
    m_roles.insert(key, value);
}

void User::setRoleMap(QMap<QString, bool> roles)
{
    m_roles = roles;
}

void User::setUserName(QString username)
{
    m_username = username;
}

void User::setDisplayName(QString displayname)
{
    m_displayname = displayname;
}

void User::setGender(GENDER gender)
{
    m_gender = gender;
}

void User::setAvatar(QString avatar)
{
    if (QFile::exists(avatar))
        m_avatar = avatar;
}

void User::setPassword(QString password)
{
    m_password = password.toUtf8();
}

void User::setPassword()
{
    if (checkNewPassword())
        m_password = m_password1;
}

void User::setNewPassword(QString password1, QString password2)
{
    m_password1 = password1.toUtf8();
    m_password2 = password2.toUtf8();
}

void User::getNewPassword(QString &password1, QString &password2)
{
    password1 = m_password1;
    password2 = m_password2;
}

bool User::checkNewPassword()
{
    return m_password1 == m_password2 && !m_password1.isEmpty();
}
