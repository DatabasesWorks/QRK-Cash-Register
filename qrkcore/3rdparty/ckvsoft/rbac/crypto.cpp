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

#include "crypto.h"

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>

#include <QString>
#include <QCryptographicHash>
#include <QDebug>

using namespace CryptoPP;

Crypto::Crypto(QObject *parent) : QObject(parent)
{
}

QString Crypto::encrypt(const SecureByteArray &plain)
{
    SecureByteArray password = plain;
    return encrypt(plain, password);
}

QString Crypto::encrypt(const SecureByteArray &plain, const SecureByteArray &password)
{
    StreamTransformationFilter::BlockPaddingScheme padding = StreamTransformationFilter::PKCS_PADDING;
    SecureByteArray key(AES::MAX_KEYLENGTH, static_cast<char>(0)), IV(AES::BLOCKSIZE, static_cast<char>(0));
    makeKeyandIvFromPassword(password, key, IV);

    CBC_Mode<AES>::Encryption enc;
    enc.SetKeyWithIV(reinterpret_cast<const byte *>(key.constData()), key.size(), reinterpret_cast<const byte *>(IV.constData()));
    const int cipherSize = (padding == StreamTransformationFilter::NO_PADDING)
                           ? plain.size()
                           : plain.size() + AES::BLOCKSIZE - plain.size() % AES::BLOCKSIZE;
    QByteArray cipher(cipherSize, static_cast<char>(0));

    try {
        ArraySource s(
                    reinterpret_cast<const byte *>(plain.constData()), plain.size(), true,
                    new StreamTransformationFilter(enc,
                                                   new ArraySink(reinterpret_cast<byte *>(cipher.data()), cipher.size()), padding)
                    );
        Q_UNUSED(s);
    } catch (Exception &e) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << e.what();
    }


    return cipher.toHex();
}

QString Crypto::decrypt(const QString &encryptedHex, const SecureByteArray &password)
{
    StreamTransformationFilter::BlockPaddingScheme padding = StreamTransformationFilter::PKCS_PADDING;
    QByteArray cipher = QByteArray::fromHex(encryptedHex.toUtf8());
    SecureByteArray key(AES::MAX_KEYLENGTH, static_cast<char>(0)), IV(AES::BLOCKSIZE, static_cast<char>(0));
    makeKeyandIvFromPassword(password, key, IV);

    CBC_Mode<AES>::Decryption dec;
    dec.SetKeyWithIV(reinterpret_cast<const byte *>(key.constData()), key.size(), reinterpret_cast<const byte *>(IV.constData()));
    SecureByteArray plain(cipher.size(), static_cast<char>(0));

    try {
        ArraySource s(
                    reinterpret_cast<const byte *>(cipher.constData()), cipher.size(), true,
                    new StreamTransformationFilter(dec,
                                                   new ArraySink(reinterpret_cast<byte *>(plain.data()), plain.size()), padding)
                    );
        Q_UNUSED(s);
    } catch (Exception &e) {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << e.what();
    }

    if (padding == StreamTransformationFilter::PKCS_PADDING)
        plain.resize(plain.size() - plain.at(plain.size() - 1));

    return plain;
}

void Crypto::makeKeyandIvFromPassword(const SecureByteArray &password, SecureByteArray &key, SecureByteArray &iv)
{
    SecureByteArray data(password);

    key = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    key.resize(AES::MAX_KEYLENGTH);
    iv = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    iv.resize(AES::BLOCKSIZE);
}

SecureByteArray Crypto::getMasterKey()
{
    return "7h15p455w0rd15m0r37h4n53cr37";
}
