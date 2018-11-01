/*
 * This file is part of ckvsoft
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
 * ckvtemplate use part of code from NLTemplate (c) 2013 tom@catnapgames.com
 * wich is MIT Licensed https://github.com/catnapgames/NLTemplate
 * and some code from Arithmetic Expression Example
 * Solution: Kristijan Burnik, Udruga informatičara Božo Težak
 *             GMail: kristijanburnik
 *
 * both are modified for QT5 use by chris@ckvsoft.at
 */

#ifndef UNIQUEMACHINEFINGERPRINT_H
#define UNIQUEMACHINEFINGERPRINT_H

#include "globals_ckvsoft.h"

#if defined(_WIN32)
#include <iphlpapi.h>
#endif

#include <QObject>

class CKVSOFT_EXPORT UniqueMachineFingerprint : public QObject
{
    Q_OBJECT
public:
    explicit UniqueMachineFingerprint(QObject *parent = nullptr);
    const QString getSystemUniqueId();
    bool validate( QString testIdString );

signals:

public slots:
private:
    quint16 getVolumeHash();
    quint16 getCpuHash();
    void getCpuid( quint32 *p, unsigned int ax);
    const QString getMachineName();

#if defined(_WIN32)
    quint16 hashMacAddress( PIP_ADAPTER_INFO info );
#else
    quint16 hashMacAddress( quint8* mac );
#if defined(__APPLE__)
    quint16 getSystemSerialNumberHash();
#else
    const QString getVolumeSerial();
#endif
#endif
    void getMacHash( quint16 &mac1, quint16 &mac2 );
    quint16 *computeSystemUniqueId();
    void smear( quint16* id );
    void unsmear( quint16* id );

};

#endif // UNIQUEMACHINEFINGERPRINT_H
