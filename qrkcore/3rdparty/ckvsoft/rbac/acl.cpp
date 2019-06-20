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

#include "acl.h"
#include "database.h"
#include "crypto.h"
#include "user.h"
#include "tempuserlogin.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDateTime>
#include <QDebug>

Acl::Acl(QObject *parent) : QObject(parent)
{
    m_timeout = QDateTime::currentDateTime();
    m_timer = new QTimer (this);
    connect (m_timer, &QTimer::timeout, this, &Acl::resetTempUserId);
    m_timer->start(1000);
}

Acl::~Acl()
{
    m_timer->stop();
}

void Acl::setuserId(int userId)
{
    m_userId = userId;
    if (userId == -1)
        return;

    m_masteradmin = isMasterAdmin();

    m_userRoles = getUserRoles(m_userId, true);
    buildAcl();

    emit userChanged();
}

void Acl::settempUserId(int userId)
{
    if (userId == -1)
        return;

    m_timeout = QDateTime::currentDateTime().addSecs(60 * 2);
    m_tempuserId = m_userId;
    m_userId = userId;

    m_masteradmin = isMasterAdmin();

    m_userRoles = getUserRoles(m_userId, true);
    buildAcl();
}

void Acl::resetTempUserId()
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 remain = now.secsTo(m_timeout);
    if (remain > 0 || m_tempuserId < 1)
        return;

    setuserId(m_tempuserId);
    m_tempuserId = -1;
}

int Acl::getUserId()
{
    return m_userId;
}

QStringList Acl::getUserRoles()
{
    return getUserRoles(m_userId);
}

QStringList Acl::getUserRoles(int id, bool byId)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    if (byId)
        query.prepare("SELECT * FROM user_roles WHERE userID = :userid ORDER BY addDate ASC");
    else
        query.prepare("SELECT roles.roleName from user_roles LEFT JOIN roles on user_roles.roleID=roles.ID WHERE userID = :userid ORDER BY addDate ASC");
    query.bindValue(":userid", id);
    query.exec();

    QStringList resp;

    while (query.next())
        resp.append((byId) ? query.value("roleId").toString() : query.value("roleName").toString());

    return resp;
}

QStringList Acl::getAllRoles()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT * FROM roles ORDER BY roleName ASC");
    query.exec();

    //    QMap<QString, int> resp;
    QStringList resp;

    while (query.next())
        resp.append(query.value("roleName").toString());
    return resp;
}

void Acl::buildAcl()
{
    //first, get the rules for the user's role
    if (m_userRoles.count() > 0)
        merge(m_perms, getRolePerms(m_userRoles));
    //then, get the individual user permissions
    merge(m_perms, getUserPerms(m_userId));
}

void Acl::merge(QMap<QString, QMap<QString, QVariant> > &map1, QMap<QString, QMap<QString, QVariant> > map2)
{
    QMap<QString, QMap<QString, QVariant> >::iterator i;
    for (i = map2.begin(); i != map2.end(); ++i) {
        QString key = i.key();
        QMap<QString, QVariant> value = i.value();
        if ((map1.contains(key) && !value.value("value").toBool()) || !map1.contains(key))
            map1.insert(key, value);
    }
}

QMap<QString, QMap<QString, QVariant> > Acl::getRolePerms(int roleid)
{
    QStringList l;

    l << QString::number(roleid);
    return getRolePerms(l);
}

QMap<QString, QMap<QString, QVariant> > Acl::getRolePerms(QStringList roleid)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    if (roleid.count() > 1) {
        QString join = roleid.join(',');

        query.prepare(QString("SELECT * FROM role_perms WHERE roleID IN (%1) ORDER BY `ID` ASC").arg(join));
        //query.bindValue(":roleids", join);
    } else if (roleid.count() == 1) {
        query.prepare("SELECT * FROM role_perms WHERE roleID = :roleid ORDER BY ID ASC");
        query.bindValue(":roleid", (roleid.count() > 0) ? roleid[0] : "-1");
    }

    query.exec();

    QMap<QString, QMap<QString, QVariant> > perms;
    bool hP;

    while (query.next()) {
        QString pK = getPermKeyFromID(query.value("permID").toInt());
        if (pK.isEmpty()) continue;
        if (query.value("value") == 1)
            hP = true;
        else
            hP = false;

        QMap<QString, QVariant> map;
        map.insert("perm", pK);
        map.insert("inheritted", true);
        map.insert("value", hP);
        map.insert("name", getPermNameFromID(query.value("permID").toInt()));
        map.insert("ID", query.value("permID").toString());
        perms.insert(pK, map);
    }
    return perms;
}

QMap<QString, QMap<QString, QVariant> > Acl::getUserPerms(int id, Acl::Format format)
{
    if (format == USER)
        return getUserPerms(id);

    QMap<QString, QMap<QString, QVariant> > perms;
    //first, get the rules for the user's role
    merge(perms, getRolePerms(getUserRoles(id, true)));

    //then, get the individual user permissions
    merge(perms, getUserPerms(id));

    return perms;
}

QMap<QString, QMap<QString, QVariant> > Acl::getUserPerms(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT * FROM user_perms WHERE userID = :id ORDER BY `addDate` ASC");
    query.bindValue(":id", id);
    query.exec();

    QMap<QString, QMap<QString, QVariant> > perms;

    while (query.next()) {
        QString pK = getPermKeyFromID(query.value("permID").toInt());
        if (pK.isEmpty())
            continue;

        bool hP;

        if (query.value("value").toBool())
            hP = true;
        else
            hP = false;

        QMap<QString, QVariant> map;
        map.insert("perm", pK);
        map.insert("inheritted", false);
        map.insert("value", hP);
        map.insert("name", getPermNameFromID(query.value("permID").toInt()));
        map.insert("ID", query.value("permID").toString());
        perms.insert(pK, map);
    }
    return perms;
}

QMap<QString, QMap<QString, QVariant> > Acl::getAllPerms()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT * FROM permissions");
    query.exec();
    QMap<QString, QMap<QString, QVariant> > perms;

    while (query.next()) {
        QString pK = query.value("permKey").toString();
        if (pK.isEmpty())
            continue;

        QMap<QString, QVariant> map;
        map.insert("ID", query.value("ID").toInt());
        map.insert("permKey", pK);
        map.insert("permName", query.value("permName").toString());
        perms.insert(pK, map);
    }

    return perms;
}

QString Acl::getPermKeyFromID(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT permKey FROM permissions WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();
    query.next();

    return query.value("permKey").toString();
}

QString Acl::getPermNameFromID(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT permName FROM permissions WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();
    query.next();

    return query.value("permName").toString();
}

int Acl::getPermIDfromKey(QString key)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT ID FROM permissions WHERE permKey = :key LIMIT 1");
    query.bindValue(":key", key);
    query.exec();
    query.next();

    return query.value("ID").toInt();
}

int Acl::getRoleIdByName(QString &roleName)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT ID FROM roles WHERE roleName = :roleName LIMIT 1");
    query.bindValue(":roleName", roleName);
    query.exec();
    if (query.next())
        return query.value("ID").toInt();

    return -1;
}

QString Acl::getRoleNameFromID(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT roleName FROM roles WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();

    return query.value("roleName").toString();
}

bool Acl::isMasterAdmin()
{
    return isMasterAdmin(m_userId);
}

bool Acl::isMasterAdmin(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT value FROM users WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
        return query.value("value").toBool();

    return false;
}

int Acl::getUserIdByName(QString name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT ID FROM users WHERE username = :name LIMIT 1");
    query.bindValue(":name", name);
    query.exec();
    query.next();

    return query.value("ID").toInt();
}

QString Acl::getUsername()
{
    return getUsername(m_userId);
}

QString Acl::getDisplayname()
{
    return getDisplayname(m_userId);
}

int Acl::getGender()
{
    return getGender(m_userId);
}

QString Acl::getAvatar()
{
    return getAvatar(m_userId);
}

QString Acl::getUsername(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT username FROM users WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();

    if (query.next()) {
        QString username = query.value("username").toString();
        if (username.isEmpty())
            username = tr("ckvsoft");
        return username;
    }

    return tr("ckvsoft");
}

QString Acl::getDisplayname(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT displayname FROM users WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();
    if (query.next()) {
        QString displayname = query.value("displayname").toString();
        if (displayname.isEmpty())
            displayname = tr("ckvsoft");
        return displayname;
    }

    return tr("ckvsoft");
}

int Acl::getGender(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT gender FROM users WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();
    if (query.next()) {
        return query.value("gender").toInt();
    }

    return 0;
}

QString Acl::getAvatar(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT avatar FROM users WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();
    if (query.next()) {
        return query.value("avatar").toString();
    }

    return "";
}

QStringList Acl::getAllUsers()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT * FROM users");
    query.exec();

    QStringList list;
    while (query.next())
        list.append(query.value("username").toString());

    return list;
}

bool Acl::userHasRole(int id)
{
    if (m_userRoles.contains(QString::number(id)))
        return m_userRoles.contains(QString::number(id));

    return false;
}

bool Acl::hasPermission(QString permKey, bool allowtempuser)
{
    if (m_userId == 0 || m_masteradmin)
        return true;


    bool access = false;
    permKey = permKey.toLower();
    if (!existPermission(permKey))
        insertPermission(permKey);

    if (m_perms.contains(permKey))
        access = m_perms.value(permKey).value("value").toBool();

    if (access)
        return true;

    if (m_userId > 0 && allowtempuser) {
        TempUserLogin *templogin = new TempUserLogin();
        if (templogin->exec() == QDialog::Accepted)
            return hasPermission(permKey);
        return false;
    }

    qDebug() << "Function Name: " << Q_FUNC_INFO << "no permissions: " << permKey;
    qDebug() << "Function Name: " << Q_FUNC_INFO << "userId: " << m_userId;

    return false;
}

bool Acl::existPermission(QString perm)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT id FROM permissions WHERE permKey = :perm LIMIT 1");
    query.bindValue(":perm", perm);
    query.exec();
    if (query.next()) {
        return true;
    }

    return false;
}

bool Acl::insertPermission(QString perm)
{
    if (existPermission(perm))
        return true;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("INSERT INTO `permissions` (ID,permKey,permName) VALUES (,:perm,:autoperm)");
    query.bindValue(":perm", perm);
    query.bindValue(":autoperm", "added by QRK: " + perm);
    query.exec();
    if (query.next()) {
        return true;
    }

    return false;
}

void Acl::saveUser(User *user, int &id)
{
    if (user->getUserName().isEmpty())
        return;

    int val = 0;
    if (getAllUsers().isEmpty())
        val = 1;

    user->setPassword();
    SecureByteArray sba = user->getPassword();
    Crypto crypto;
    crypto.encrypt(sba);
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("UPDATE users SET username = :name, displayname = :displayname, password = :password, gender = :gender, avatar = :avatar, addDate = :date WHERE ID = :id");
    query.bindValue(":id", id);
    query.bindValue(":name", user->getUserName());
    query.bindValue(":password", (sba.isEmpty() ? getPasswordByUserId(id) : crypto.encrypt(sba)));
    query.bindValue(":displayname", user->getDisplayName());
    query.bindValue(":gender", int(user->getGender()));
    query.bindValue(":avatar", user->getAvatar());
    query.bindValue(":date", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.exec();
    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO users (username, displayname, password, value, gender, avatar, addDate) VALUES(:name, :displayname, :password, :value, :gender, :avatar, :date)");
        query.bindValue(":name", user->getUserName());
        query.bindValue(":displayname", user->getDisplayName());
        query.bindValue(":gender", int(user->getGender()));
        query.bindValue(":avatar", user->getAvatar());
        query.bindValue(":password", crypto.encrypt(sba));
        query.bindValue(":value", val);
        query.bindValue(":date", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.exec();
    }

    query.prepare("SELECT ID FROM users WHERE username = :name");
    query.bindValue(":name", user->getUserName());
    query.exec();
    if (query.next())
        id = query.value("ID").toInt();

    QMapIterator<QString, QMap<QString, QVariant> > i(user->getPermissions());
    while (i.hasNext()) {
        i.next();
        QMap<QString, QVariant> perms = i.value();
        int permID = perms.value("ID").toInt();
        if (i.value().value("inheritted").toBool()) {
            query.prepare("DELETE FROM user_perms WHERE userID = :userID AND permID = :permID");
            query.bindValue(":userID", id);
            query.bindValue(":permID", permID);
            query.exec();
            continue;
        }
        query.prepare("REPLACE INTO user_perms (userID, permID, value, addDate) VALUES(:userID, :permID, :value, :date)");
        query.bindValue(":userID", id);
        query.bindValue(":permID", permID);
        query.bindValue(":value", i.value().value("value").toBool());
        query.bindValue(":date", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.exec();
    }

    QMapIterator<QString, bool> r(user->getRoles());
    while (r.hasNext()) {
        r.next();
        QString key = r.key();
        int roleID = getRoleIdByName(key);
        if (!r.value()) {
            query.prepare("DELETE FROM user_roles WHERE userID = :userID AND roleID = :roleID");
            query.bindValue(":userID", id);
            query.bindValue(":roleID", roleID);
            query.exec();
            continue;
        }
        query.prepare("REPLACE INTO user_roles (userID, roleID, addDate) VALUES(:userID, :roleID, :date)");
        query.bindValue(":userID", id);
        query.bindValue(":roleID", roleID);
        query.bindValue(":date", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.exec();
    }
}

void Acl::deleteUser(QString &name, int id)
{
    if (name.isEmpty() || id == -1)
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    bool ok;
    query.prepare("DELETE FROM users WHERE ID = :id AND username = :name LIMIT 1");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    ok = query.exec();
    if (!ok)
        qWarning() << "Function Name: " << Q_FUNC_INFO << " DELETE FROM users: " << ok;

    query.prepare("DELETE FROM user_roles WHERE userID = :id");
    query.bindValue(":id", id);
    ok = query.exec();
    if (!ok)
        qWarning() << "Function Name: " << Q_FUNC_INFO << " DELETE FROM user_roles: " << ok;

    query.prepare("DELETE FROM user_perms WHERE userID = :id");
    query.bindValue(":id", id);
    query.exec();
    if (!ok)
        qWarning() << "Function Name: " << Q_FUNC_INFO << " DELETE FROM user_perms: " << ok;
}

void Acl::saveRole(QString &name, int &id, QMap<QString, QMap<QString, QVariant> > &rolePerms)
{
    if (name.isEmpty())
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("UPDATE roles SET roleName = :name WHERE ID = :id");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.exec();
    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO roles (roleName) VALUES(:name)");
        query.bindValue(":name", name);
        query.exec();
    }

    query.prepare("SELECT ID FROM roles WHERE roleName = :name");
    query.bindValue(":name", name);
    query.exec();
    if (query.next())
        id = query.value("ID").toInt();

    QMapIterator<QString, QMap<QString, QVariant> > i(rolePerms);
    while (i.hasNext()) {
        i.next();
        QMap<QString, QVariant> perms = i.value();
        int permID = perms.value("ID").toInt();
        if (i.value().value("ignore").toBool()) {
            query.prepare("DELETE FROM role_perms WHERE roleID = :roleID AND permID = :permID");
            query.bindValue(":roleID", id);
            query.bindValue(":permID", permID);
            query.exec();
            continue;
        }
        query.prepare("REPLACE INTO role_perms (roleID, permID, value, addDate) VALUES(:roleID, :permID, :value, :date)");
        query.bindValue(":roleID", id);
        query.bindValue(":permID", permID);
        query.bindValue(":value", i.value().value("value").toBool());
        query.bindValue(":date", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.exec();
    }
}

void Acl::deleteRole(QString &name, int id)
{
    if (name.isEmpty() || id == -1)
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    bool ok;
    query.prepare("DELETE FROM roles WHERE ID = :id AND roleName = :name LIMIT 1");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    ok = query.exec();
    if (!ok)
        qWarning() << "Function Name: " << Q_FUNC_INFO << " DELETE FROM roles: " << ok;

    query.prepare("DELETE FROM user_roles WHERE roleID = :id");
    query.bindValue(":id", id);
    ok = query.exec();
    if (!ok)
        qWarning() << "Function Name: " << Q_FUNC_INFO << " DELETE FROM user_roles: " << ok;

    query.prepare("DELETE FROM role_perms WHERE roleID = :id");
    query.bindValue(":id", id);
    query.exec();
    if (!ok)
        qWarning() << "Function Name: " << Q_FUNC_INFO << " DELETE FROM role_perms: " << ok;
}

void Acl::savePerms(QMap<QString, QMap<QString, QVariant> > &permissions)
{
    if (permissions.isEmpty())
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    QMapIterator<QString, QMap<QString, QVariant> > i(permissions);
    while (i.hasNext()) {
        i.next();
        QMap<QString, QVariant> perms = i.value();
        int id = perms.value("ID").toInt();
        QString permKey = perms.value("permKey").toString();
        QString permName = perms.value("permName").toString();
        query.prepare("REPLACE INTO permissions (ID, permKey, permName) VALUES(:ID, :permKey, :permName)");
        query.bindValue(":ID", id);
        query.bindValue(":permKey", permKey);
        query.bindValue(":permName", permName);
        query.exec();
    }
}

QString Acl::getPasswordByUserName(QString &name)
{
    int id = getUserIdByName(name);

    return getPasswordByUserId(id);
}

QString Acl::getPasswordByUserId(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT password FROM users WHERE ID = :id LIMIT 1");
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
        return query.value("password").toString();

    return "";
}

bool Acl::Login()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT username FROM users LIMIT 1");
    query.exec();
    if (query.next())
        return true;

    return false;
}
