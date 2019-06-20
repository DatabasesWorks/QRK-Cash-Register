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

#ifndef CRYPTO_H
#define CRYPTO_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"

#include <QObject>
#include <QByteArray>

class SecureByteArray;

class CKVSOFT_EXPORT Crypto : QObject
{
        Q_OBJECT

    public:
        Crypto(QObject *parent = Q_NULLPTR);
        static QString encrypt(const SecureByteArray &plain);
        static QString encrypt(const SecureByteArray &plain, const SecureByteArray &password);
        static QString decrypt(const QString &encryptedHex, const SecureByteArray &password);

        static void makeKeyandIvFromPassword(const SecureByteArray &password, SecureByteArray &key, SecureByteArray &iv);
        SecureByteArray getMasterKey();

};

class CKVSOFT_EXPORT SecureByteArray : public QByteArray
{
    public:
        SecureByteArray(void){}
        SecureByteArray(const char *data, int size = -1): QByteArray(data, size) {}
        SecureByteArray(int size, char ch)  : QByteArray(size, ch) {}
        SecureByteArray(const QByteArray &other)  : QByteArray(other) {}
        ~SecureByteArray(){invalidate();}

        void invalidate(void){SecureErase(*this);}
        void SecureErase(QString str)
        {
            for (QString::iterator i = str.begin(); i != str.end(); ++i)
                *i = 0;
            str.clear();
        }
};

#endif // CRYPTO_H
