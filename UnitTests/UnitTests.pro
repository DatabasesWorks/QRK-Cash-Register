#
# This file is part of QRK - Qt Registrier Kasse
#
# Copyright (C) 2015-2019 Christian Kvasny <chris@ckvsoft.at>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
# Button Design, and Idea for the Layout are lean out from LillePOS, Copyright 2010, Martin Koller, kollix@aon.at
#

include(../defaults.pri)

TARGET = QRK-UnitTests

TEMPLATE = app qt

QT += core testlib
QT -= gui

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#CONFIG += console warn_off testcase no_testcase_installs
#QMAKE_LFLAGS += -Wl,--rpath=../qrkcore
CONFIG += console warn_off no_testcase_installs

SOURCES += test-main.cpp

HEADERS +=

INCLUDEPATH += $$SRC_DIR/qrkcore
DEPENDPATH += $$SRC_DIR/qrkcore

DEFINES += QT_DEPRECATED_WARNINGS

win32:CONFIG(release, debug|release): LIBS += -L../qrkcore/release -lQrkCore
else:win32:CONFIG(debug, debug|release): LIBS += -L../qrkcore/debug -lQrkCore
else:unix: LIBS += -L../qrkcore -lQrkCore

unix:!macx {
 INCLUDEPATH += /usr/include/PCSC
 LIBS += -lpcsclite
}

macx {
 INCLUDEPATH += /usr/local/include
 #INCLUDEPATH += /usr/local/Cellar/qrencode/3.4.4/include
 QMAKE_LFLAGS += -Wl,-rpath,@executable_path/
 LIBS += -L/usr/local/lib
 LIBS += -framework PCSC
 LIBS += -framework CoreFoundation
 ICON = ../qrk.icns
}

win32 {
 INCLUDEPATH += $$[QT_INSTALL_PREFIX]/include/QtZlib
 LIBS += libwinscard
 LIBS += -pthread
 RC_ICONS = ../qrk.ico
}

LIBS += -lcryptopp
LIBS += -lz
