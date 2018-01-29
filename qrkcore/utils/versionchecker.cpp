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

#include "versionchecker.h"
#include "database.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

VersionChecker::VersionChecker(QObject *parent) :
    QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
    m_url = "http://service.ckvsoft.at/qrk";
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VersionChecker::getVersion);
#if defined(_WIN32) || defined(__APPLE__)
    timer->start(14400000);
#else
    timer->start(2000);
#endif
}

VersionChecker::~VersionChecker()
{
    delete m_manager;
    timer->stop();
}

void VersionChecker::getVersion()
{
    timer->stop();
    QUrl reqUrl(m_url + "/version.php");
    QNetworkRequest req(reqUrl);

    //Creating the JSON-Data
    QJsonObject *jsondata = new QJsonObject();
    jsondata->insert("request", "POST");
    jsondata->insert("version", qApp->applicationVersion());
    jsondata->insert("product", QSysInfo::prettyProductName());
    jsondata->insert("shop", Database::getShopName());
    jsondata->insert("registerId", Database::getCashRegisterId());
    jsondata->insert("kernel", QSysInfo::kernelType());

    req.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));
    req.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(QJsonDocument(*jsondata).toJson().size()));

    if (doRequest(req, *jsondata)) {
        if (jsondata->value("version").toString() > qApp->applicationVersion())
            emit Version(jsondata->value("version").toString());
        qDebug() << "Function Name: " << Q_FUNC_INFO << " Version: " << jsondata->value("version").toString();
    }

    timer->start(14400000);
}

bool VersionChecker::doRequest(QNetworkRequest req, QJsonObject &obj)
{

    // Connection via HTTPS
    QSslSocket* sslSocket = new QSslSocket(this);
    QSslConfiguration configuration = req.sslConfiguration();
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    configuration.setProtocol(QSsl::AnyProtocol);

    sslSocket->setSslConfiguration(configuration);
    req.setSslConfiguration(configuration);

    QEventLoop eventLoop;

    //Sending the Request
    QString request = obj.value("request").toString();
    QNetworkReply *reply;

    if (request == "POST")
        reply = m_manager->post(req, QJsonDocument(obj).toJson());
    else if (request == "PUT")
        reply = m_manager->put(req, QJsonDocument(obj).toJson());
    else if (request == "GET")
        reply = m_manager->get(req);
    else if (request == "DELETE")
        reply = m_manager->deleteResource(req);
    else
        return false;

    QObject::connect(m_manager, &QNetworkAccessManager::finished, &eventLoop, &QEventLoop::quit);
    // the HTTP request
    eventLoop.exec(); // blocks stack until "finished()" has been called

    if (reply->error() == QNetworkReply::NoError) {
        //success
        obj = QJsonDocument::fromJson(reply->readAll()).object();
        delete reply;
        return true;
    }
    else {
        //failure
        obj["errorstring"] = reply->errorString();
        delete reply;
        return false;
    }
    return false;
}
