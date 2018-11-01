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

#include "3rdparty/ckvsoft/rbac/crypto.h"

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>

#include "RK/rk_signaturemodule.h"
#include "RK/rk_signaturemodulefactory.h"

#include "3rdparty/qbcmath/bcmath.h"

#include <QDebug>
#include <QDir>
#include <QMessageAuthenticationCode>
#include <QtTest/QTest>

class QRK : public QObject
{
        Q_OBJECT

    private slots:
        void crypto_make_key(void)
        {
            const SecureByteArray masterPassword = QString("7h15p455w0rd15m0r37h4n53cr37").toUtf8();
            SecureByteArray key;
            SecureByteArray iv;
            Crypto::makeKeyandIvFromPassword(masterPassword, key, iv);

            QVERIFY(key.size() == CryptoPP::AES::MAX_KEYLENGTH);
            QVERIFY(key.toBase64() == "yXoW7/KiUiO+KOeBmyKDEBQCXRkbBb/3c+xlU0SXBEM=");
            QVERIFY(iv.size() == CryptoPP::AES::BLOCKSIZE);
            QVERIFY(iv.toBase64() == "OVz9yELbX20s9iHogmDPWg==");
        }

        void base32_encode_decode(void)
        {
            QByteArray teststr = QString("Das ist ein Base32 encode Test").toUtf8();
            QByteArray encode = RKSignatureModule::base32_encode(teststr);
            QByteArray decode = RKSignatureModule::base32_decode(encode);
            QVERIFY(decode == teststr);
        }

        void decode_AES256Key(void)
        {
            const SecureByteArray ba("cmXWiKzUqJ8/bq0OWOTKkV6i0G0AIyz24pZElTQD0hc=");
            SecureByteArray::fromBase64(ba);
            qDebug() << "ba hex: " << SecureByteArray::fromBase64(ba).toHex();
            QVERIFY("7265d688acd4a89f3f6ead0e58e4ca915ea2d06d00232cf6e29644213403d217" == SecureByteArray::fromBase64(ba).toHex());
        }

        void encrypt_decrypt_TurnoverCounter(void)
        {
            const SecureByteArray masterPassword = QString("7h15p455w0rd15m0r37h4n53cr37").toUtf8();
            SecureByteArray key;
            SecureByteArray iv;
            Crypto::makeKeyandIvFromPassword(masterPassword, key, iv);
            QString concatenated = "concatenated";
            qlonglong turnoverCounter = -12345;
            RKSignatureModule *module = RKSignatureModuleFactory::createInstance("", true);
            QString encrypt = module->encryptTurnoverCounter( concatenated, turnoverCounter, key.toHex());
            QString decrypt = module->decryptTurnoverCounter( concatenated, encrypt, key.toHex());
            QVERIFY(turnoverCounter == decrypt.toLongLong());

            turnoverCounter = 34567;
            encrypt = module->encryptTurnoverCounter( concatenated, turnoverCounter, key.toHex());
            decrypt = module->decryptTurnoverCounter( concatenated, encrypt, key.toHex());
            QVERIFY(turnoverCounter == decrypt.toLongLong());

            delete module;
        }

        void bcmath(void)
        {
            QString test = "63.99";
            QBCMath m(63.985);
            m.round(2);
            qDebug() << "bcmath: " << m.toString();
            QVERIFY(m.toString() == test);
            double x = QString::number(1234567890.27000000003, 'f').toDouble();
            QBCMath y(x);
            QVERIFY(y.toDouble() == x);
        }

        void datetime(void)
        {
            QTime time = QTime(4,30,0);
            QDateTime dt = QDateTime::currentDateTime().addSecs(-QTime(0, 0, 0).secsTo(time));
            QDate date = QDate::currentDate();
            qDebug() << "time: " << time << " datetime: " << dt << " datetime date: " << dt.date() << " date: " << date;
            QVERIFY(dt.date() == date);
        }


};

QTEST_GUILESS_MAIN(QRK)
#include "test-main.moc"
