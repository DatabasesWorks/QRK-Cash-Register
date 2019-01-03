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

#include "qrkmultimedia.h"
#include "preferences/qrksettings.h"
#include <QApplication>
#include <QDir>
#include <QtMultimedia/QSound>
#include <QThread>
#include <QDebug>

QrkMultimedia::QrkMultimedia(QObject *parent) : QObject(parent)
{

}

QStringList QrkMultimedia::getMultimediaFiles(QString path)
{
    QDir directory(getMultimediaPath(path));
    return directory.entryList(QStringList() << "*.wav", QDir::Files);
}

QString QrkMultimedia::getMultimediaPath(QString path)
{
    if (path.isEmpty()) {
        QrkSettings settings;
        return settings.value("multimediadirectory", qApp->applicationDirPath()).toString() + QDir::separator();
    }
    return path;
}

void QrkMultimedia::play(QRKMULTIMEDIA what)
{
    QrkSettings settings;
    settings.beginGroup("BarcodeReader");
    QFile soundfile;
    switch(what) {
    case BARCODE_SUCCSESS:
        if (settings.value("barcode_success_enabled", false).toBool()) {
            soundfile.setFileName(settings.value("barcode_success_sound").toString());
            if (soundfile.exists())
                play(soundfile.fileName());
            else
                play(":src/multimedia/success.wav");
        }
        break;
    case BARCODE_FAILURE:
        if (settings.value("barcode_failure_enabled", false).toBool()) {
            soundfile.setFileName(settings.value("barcode_failure_sound").toString());
            if (soundfile.exists())
                play(soundfile.fileName());
            else
                play(":src/multimedia/failure.wav");
        }
        break;
    }
    settings.endGroup();
}

void QrkMultimedia::play(QString soundfile)
{
    if (soundfile.isEmpty()) return;
    QSound::play(soundfile);
}
