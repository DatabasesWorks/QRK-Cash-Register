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
#

INCLUDEPATH += $$SRC_DIR/qrkcore/

SOURCES += \ 
    $$PWD/qrkgastro.cpp \
    $$PWD/qrkroomtablebuttons.cpp \
    $$PWD/qrkgastroselector.cpp \
    $$PWD/qrkgastrotableorder.cpp \
    $$PWD/qrkgastroopenticketslistwidget.cpp \
    $$PWD/qrkgastroopenticketwidget.cpp \
    $$PWD/qrkgastroopentickets.cpp \
    $$PWD/qrkgastrosplitticketwidget.cpp \
    $$PWD/qrkgastrofinishticketdialog.cpp \
    $$PWD/qrkgastrofinishticket.cpp \
    $$PWD/qrkgastromanagers/qrkgastrotablemanager.cpp \
    $$PWD/qrkgastromanagers/qrkgastromanagerroomedit.cpp \
    $$PWD/qrkgastromanagers/qrkgastromanagertableedit.cpp \
    $$PWD/qrkgastrovoiddialog.cpp \
    $$PWD/qrkgastroquickproduct.cpp \
    $$PWD/qrkgastrocurfewchecker.cpp

HEADERS += \ 
    $$PWD/qrkgastro.h \
    $$PWD/qrkroomtablebuttons.h \
    $$PWD/qrkgastroselector.h \
    $$PWD/qrkgastrotableorder.h \
    $$PWD/qrkgastroopenticketslistwidget.h \
    $$PWD/qrkgastroopenticketwidget.h \
    $$PWD/qrkgastroopentickets.h \
    $$PWD/qrkgastrosplitticketwidget.h \
    $$PWD/qrkgastrofinishticketdialog.h \
    $$PWD/qrkgastrofinishticket.h \
    $$PWD/qrkgastromanagers/qrkgastrotablemanager.h \
    $$PWD/qrkgastromanagers/qrkgastromanagerroomedit.h \
    $$PWD/qrkgastromanagers/qrkgastromanagertableedit.h \
    $$PWD/qrkgastrovoiddialog.h \
    $$PWD/qrkgastroquickproduct.h \
    $$PWD/qrkgastrocurfewchecker.h

FORMS += \ 
    $$PWD/qrkgastro.ui \
    $$PWD/qrkgastroselector.ui \
    $$PWD/qrkgastrotableorder.ui \
    $$PWD/qrkgastroopentickets.ui \
    $$PWD/qrkgastrosplitticketwidget.ui \
    $$PWD/qrkgastrofinishticketdialog.ui \
    $$PWD/qrkgastromanagers/qrkgastrotablemanager.ui \
    $$PWD/qrkgastromanagers/qrkgastromanagerroomedit.ui \
    $$PWD/qrkgastromanagers/qrkgastromanagertableedit.ui \
    $$PWD/qrkgastrovoiddialog.ui \
    $$PWD/qrkgastroquickproduct.ui

