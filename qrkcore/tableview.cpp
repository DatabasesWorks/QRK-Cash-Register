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

#include "tableview.h"

#include <QToolTip>
#include <QEvent>
#include <QHelpEvent>

TableView::TableView(QWidget* parent)
    : QTableView(parent)
{
    this->setMouseTracking(true);
}

void TableView::showOrUpdateToolTip()
{
    if (QTableView::underMouse() && _isActive) {
        const QModelIndex index = QTableView::indexAt(_lastPosition);
        if (index.isValid()) {
            const QString toolTip = model()->data(model()->index(index.row(), 0), Qt::ToolTipRole).toString();
            QToolTip::showText(_lastGlobalPosition, toolTip, this);
        }
    }
}

void TableView::mouseMoveEvent(QMouseEvent * event)
{
    _isActive = false;
    QToolTip::hideText();

    QTableView::mouseMoveEvent(event);
}

bool TableView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        _lastPosition = static_cast<QHelpEvent*>(event)->pos();
        _lastGlobalPosition = static_cast<QHelpEvent*>(event)->globalPos();
        _isActive = true;
        showOrUpdateToolTip();
        return true;
    }
    return QTableView::viewportEvent(event);
}
