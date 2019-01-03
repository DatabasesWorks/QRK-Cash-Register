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

#ifndef QRKCORE_GLOBAL
#define QRKCORE_GLOBAL

#include <QtCore/qglobal.h>

#ifdef QRK_STATIC
#define QRK_EXPORT
#else
#if defined(QRK_BUILD)
#define QRK_EXPORT Q_DECL_EXPORT
#else
#define QRK_EXPORT Q_DECL_IMPORT
#endif
#endif // QRK_STATIC

#ifndef Q_FALLTHROUGH
#ifndef QT_HAS_CPP_ATTRIBUTE
#ifdef __has_cpp_attribute
#  define QT_HAS_CPP_ATTRIBUTE(x)       __has_cpp_attribute(x)
#else
#  define QT_HAS_CPP_ATTRIBUTE(x)       0
#endif
#endif
#if defined(__cplusplus)
#if QT_HAS_CPP_ATTRIBUTE(fallthrough)
#  define Q_FALLTHROUGH() [[fallthrough]]
#elif QT_HAS_CPP_ATTRIBUTE(clang::fallthrough)
#    define Q_FALLTHROUGH() [[clang::fallthrough]]
#elif QT_HAS_CPP_ATTRIBUTE(gnu::fallthrough)
#    define Q_FALLTHROUGH() [[gnu::fallthrough]]
#endif
#endif
#ifndef Q_FALLTHROUGH
#  if (defined(Q_CC_GNU) && Q_CC_GNU >= 700) && !defined(Q_CC_INTEL)
#    define Q_FALLTHROUGH() __attribute__((fallthrough))
#  else
#    define Q_FALLTHROUGH() (void)0
#endif
#endif
#endif

#endif // QRKCORE_GLOBAL

