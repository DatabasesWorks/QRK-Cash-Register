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

#include "horizontalscrollarea.h"


HorizontalScrollArea::HorizontalScrollArea(int rows, int cols, QWidget *parent)
    :QScrollArea(parent), nRows(rows), nColumns(cols)
{
    setWidgetResizable(true);
    contentWidget = new QWidget(this);
    setWidget(contentWidget);
    grid = new QGridLayout(contentWidget);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void HorizontalScrollArea::addWidget(QWidget *w, int row, int col){
    grid->addWidget(w, row, col);
    adaptSize();
}

int HorizontalScrollArea::columnCount() const{
    if(grid->count() == 0){
        return 0;
    }
    return grid->columnCount();
}

void HorizontalScrollArea::adaptSize(){
    if(columnCount() >= nColumns ){
        int w = 1.0*(width() - grid->horizontalSpacing()*(nColumns+1.6))/nColumns;
        int wCorrected = w*columnCount() + grid->horizontalSpacing()*(columnCount()+2);
        contentWidget->setFixedWidth(wCorrected);
    }
    contentWidget->setFixedHeight(viewport()->height());
}

void HorizontalScrollArea::resizeEvent(QResizeEvent *event){
    QScrollArea::resizeEvent(event);
    adaptSize();
}

void HorizontalScrollArea::clear()
{
    while (grid->count() > 0) delete grid->takeAt(0);
}
