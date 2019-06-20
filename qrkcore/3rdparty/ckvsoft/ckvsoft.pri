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

INCLUDEPATH += "$$PWD"

HEADERS += $$PWD/ckvtemplate.h \
    $$PWD/globals_ckvsoft.h \
    $$PWD/rbac/acl.h \
    $$PWD/rbac/crypto.h \
    $$PWD/rbac/useradmin.h \
    $$PWD/rbac/rolesadmin.h \
    $$PWD/rbac/aclmanager.h \
    $$PWD/rbac/permissionsadmin.h \
    $$PWD/rbac/aclwizard.h \
    $$PWD/flowlayout.h \
    $$PWD/rbac/user.h \
    $$PWD/rbac/userlogin.h \
    $$PWD/rbac/resetpassword.h \
    $$PWD/rbac/base_login.h \
    $$PWD/rbac/tempuserlogin.h \
    $$PWD/uniquemachinefingerprint.h \
    $$PWD/numerickeypad.h \
    $$PWD/headerview.h \
    $$PWD/qsqlrtmodel.h

SOURCES += $$PWD/ckvtemplate.cpp \
    $$PWD/rbac/acl.cpp \
    $$PWD/rbac/crypto.cpp \
    $$PWD/rbac/useradmin.cpp \
    $$PWD/rbac/rolesadmin.cpp \
    $$PWD/rbac/aclmanager.cpp \
    $$PWD/rbac/permissionsadmin.cpp \
    $$PWD/rbac/aclwizard.cpp \
    $$PWD/flowlayout.cpp \
    $$PWD/rbac/user.cpp \
    $$PWD/rbac/userlogin.cpp \
    $$PWD/rbac/resetpassword.cpp \
    $$PWD/rbac/base_login.cpp \
    $$PWD/rbac/tempuserlogin.cpp \
    $$PWD/uniquemachinefingerprint.cpp \
    $$PWD/numerickeypad.cpp \
    $$PWD/headerview.cpp

FORMS += \
    $$PWD/rbac/base_admin.ui

win32 {
    LIBS += -liphlpapi
}
unix:!macx {
    LIBS += -ludev
}
