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

#include "quickbuttons.h"
#include "drag/dragflowwidget.h"
#include "drag/dragpushbutton.h"
#include "ui_quickbuttons.h"

#include <QScrollBar>
#include <QSize>
#include <QDebug>

QuickButtons::QuickButtons(QWidget *parent) :
    QWidget(parent), ui(new Ui::QuickButtons)
{
    ui->setupUi(this);
    ui->boxLayout->setDirection(QBoxLayout::Direction::LeftToRight);
}

QuickButtons::~QuickButtons()
{
    delete ui;
}

void QuickButtons::refresh()
{
    readSettings();
    quickTopButtons();
    if (isTopBoxHidden()) {
        quickMiddleButtons();
    }
}

void QuickButtons::setBoxName(BoxPosition position, const QString &title)
{
    switch (position) {
    case BoxPosition::TOP :
        ui->topBox->setTitle(title);
        break;
    case BoxPosition::MIDDLE :
        ui->middleBox->setTitle(title);
        break;
    case BoxPosition::BOTTOM :
        ui->bottomBox->setTitle(title);
        break;
    }
}

void QuickButtons::upPushButton(bool clicked)
{
    Q_UNUSED(clicked);
    int value = (ui->middleScrollArea->isHidden())? ui->bottomScrollArea->verticalScrollBar()->value():ui->middleScrollArea->verticalScrollBar()->value();
    int pagestep = (ui->middleScrollArea->isHidden())? ui->bottomScrollArea->verticalScrollBar()->pageStep(): ui->middleScrollArea->verticalScrollBar()->pageStep();
    if (ui->middleScrollArea->isHidden())
        ui->bottomScrollArea->verticalScrollBar()->setValue(value - pagestep);
    else
        ui->middleScrollArea->verticalScrollBar()->setValue(value - pagestep);
}

void QuickButtons::downPushButton(bool clicked)
{
    Q_UNUSED(clicked);
    int value = (ui->middleScrollArea->isHidden())? ui->bottomScrollArea->verticalScrollBar()->value():ui->middleScrollArea->verticalScrollBar()->value();
    int pagestep = (ui->middleScrollArea->isHidden())? ui->bottomScrollArea->verticalScrollBar()->pageStep(): ui->middleScrollArea->verticalScrollBar()->pageStep();
    if (ui->middleScrollArea->isHidden())
        ui->bottomScrollArea->verticalScrollBar()->setValue(value + pagestep);
    else
        ui->middleScrollArea->verticalScrollBar()->setValue(value + pagestep);
}

void QuickButtons::updateSortOrderProducts(const QList<int> &indexList)
{
    m_sortOrderBotBox = indexList;
}

void QuickButtons::updateSortOrderGroups(const QList<int> &indexList)
{
    m_sortOrderMidBox = indexList;
}

void QuickButtons::setTopWidget(DragFlowWidget *widget)
{
    ui->topScrollArea->setWidget(widget);
}

DragFlowWidget *QuickButtons::getTopWidget()
{
    return static_cast<DragFlowWidget*>(ui->topScrollArea->widget());
}

void QuickButtons::setMidWidget(DragFlowWidget *widget)
{
    ui->middleScrollArea->setWidget(widget);
}

DragFlowWidget *QuickButtons::getMidWidget()
{
    return static_cast<DragFlowWidget*>(ui->middleScrollArea->widget());
}

void QuickButtons::setBotWidget(DragFlowWidget *widget)
{
    ui->bottomScrollArea->setWidget(widget);
}

DragFlowWidget *QuickButtons::getBotWidget()
{
    return static_cast<DragFlowWidget*>(ui->bottomScrollArea->widget());
}

void QuickButtons::setStretchFactor(QSize stretchFactor)
{
    ui->boxLayout->setStretch(1, stretchFactor.width());
    ui->boxLayout->setStretch(2, stretchFactor.height());
}

void QuickButtons::setDirection(int direction)
{
    ui->boxLayout->setDirection(QBoxLayout::Direction(direction));
}

void QuickButtons::setTopBoxHidden(bool hidden)
{
    m_topBoxHidden = hidden;
    ui->topBox->setHidden(m_topBoxHidden);
    ui->topBox->setVisible(!m_topBoxHidden);
}

bool QuickButtons::isTopBoxHidden()
{
    return m_topBoxHidden;
}

void QuickButtons::setMiddleBoxHidden(bool hidden)
{
    m_middleBoxHidden = hidden;
    ui->middleBox->setHidden(m_middleBoxHidden);
}

void QuickButtons::setBottomBoxHidden(bool hidden)
{
    m_bottomBoxHidden = hidden;
    ui->bottomBox->setHidden(m_bottomBoxHidden);
}

bool QuickButtons::isMiddleBoxHidden()
{
    return m_middleBoxHidden;
}

bool QuickButtons::isBottomBoxHidden()
{
    return m_bottomBoxHidden;
}

void QuickButtons::setQuickButtonSize(QSize size)
{
    m_quickButtonSize = size;
}

QSize QuickButtons::getQuickButtonSize()
{
    return m_quickButtonSize;
}

QList<int> QuickButtons::getSortOrderList(BoxPosition order, bool clear)
{
    QList<int> orderlist;
    if(order == BoxPosition::TOP) {
        orderlist = m_sortOrderTopBox;
        if (clear)
            m_sortOrderTopBox.clear();
    } else if (order == BoxPosition::MIDDLE){
        orderlist = m_sortOrderMidBox;
        if (clear)
            m_sortOrderMidBox.clear();
    } else /* BoxPosition::BOTTOM */ {
    orderlist = m_sortOrderBotBox;
    if (clear)
        m_sortOrderBotBox.clear();
    }

    return orderlist;
}
