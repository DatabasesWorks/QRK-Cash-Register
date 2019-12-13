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

#ifndef QRKQUICKBUTTONS_H
#define QRKQUICKBUTTONS_H

#include "qrkcore_global.h"
#include "3rdparty/ckvsoft/quickbuttons.h"

class DragPushButton;

class CKVSOFT_EXPORT QrkQuickButtons : public QuickButtons
{
    Q_OBJECT

public:
    explicit QrkQuickButtons(QWidget *parent = Q_NULLPTR);

signals:
    void addProductToOrderList(int id);

public slots:
    void backToTopButton(bool clicked) override;
    void backToMiddleButton(bool clicked) override;
    void quickBottomButtons(int id) override;

private:
    void quickTopButtons() override;
    void quickMiddleButtons(int id = 1) override;
    DragPushButton *getCategorieButton(int id);
    DragPushButton *getGroupButton(int id);

    void readSettings() override;

    bool m_usecategories = false;
    bool m_categoriesbuttonsettings = false;
    bool m_groupbuttonsettings = false;
    int m_currentGroupId = 0;
    int m_currentCategorieId = 0;
};

#endif // QRKQUICKBUTTONS_H
