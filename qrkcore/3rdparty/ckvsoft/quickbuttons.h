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

#ifndef QUICKBUTTONS_H
#define QUICKBUTTONS_H

#include "globals_ckvsoft.h"
#include <QWidget>

class DragFlowWidget;

namespace Ui {
class QuickButtons;
}

class CKVSOFT_EXPORT QuickButtons : public QWidget
{
    Q_OBJECT

public:
    explicit QuickButtons(QWidget *parent = Q_NULLPTR);
    ~QuickButtons();

protected:
    enum BoxPosition {
        TOP = 0,
        MIDDLE,
        BOTTOM
    };

    void setTopWidget(DragFlowWidget *widget);
    DragFlowWidget *getTopWidget();
    void setMidWidget(DragFlowWidget *widget);
    DragFlowWidget *getMidWidget();
    void setBotWidget(DragFlowWidget *widget);
    DragFlowWidget *getBotWidget();
    void setBoxName(BoxPosition pos, const QString &title);

    void setStretchFactor(QSize stretchFactor);
    void setDirection(int direction);
    void setTopBoxHidden(bool hidden);
    void setMiddleBoxHidden(bool hidden);
    void setBottomBoxHidden(bool hidden);
    bool isTopBoxHidden();
    bool isMiddleBoxHidden();
    bool isBottomBoxHidden();
    void setQuickButtonSize(QSize size);
    QSize getQuickButtonSize();
    QList<int> getSortOrderList(BoxPosition order, bool clear = false);

signals:
    void showCategoriePushButton(bool hidden);
    void enableCategoriePushButton(bool hidden);

public slots:
    void refresh();
    void upPushButton(bool clicked);
    void downPushButton(bool clicked);
    void updateSortOrderGroups(const QList<int> &indexList);
    void updateSortOrderProducts(const QList<int> &indexList);
    virtual void backToTopButton(bool clicked) = 0;
    virtual void backToMiddleButton(bool clicked) = 0;
    virtual void quickTopButtons() = 0;
    virtual void quickMiddleButtons(int id = 1) = 0;
    virtual void quickBottomButtons(int id) = 0;

private:
    Ui::QuickButtons *ui;

    virtual void readSettings() = 0;
    QSize m_quickButtonSize;
    QList<int> m_sortOrderTopBox;
    QList<int> m_sortOrderMidBox;
    QList<int> m_sortOrderBotBox;

    bool m_topBoxHidden = false;
    bool m_middleBoxHidden = false;
    bool m_bottomBoxHidden = false;

};

#endif // QUICKBUTTONS_H
