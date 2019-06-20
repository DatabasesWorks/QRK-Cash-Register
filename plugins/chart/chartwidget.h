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

#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QtPlugin>

#define PerConst = 3.6;

class pieceNC
{
    public:
        explicit pieceNC();
        void addName(QString name);
        void setColor(Qt::GlobalColor);
        void setColor(QColor color);
        void setPerc(float Percentage);

        QString pname;
        QColor rgbColor;
        float pPerc;

    private:

};

class ChartWidget : public QWidget
{
        Q_OBJECT

    public:

        enum PIETYPE { Histogramm , Pie, Dpie };
        enum LEGENDTYPE{ /*Horizontal,*/ Vertical, Round, None };

        explicit ChartWidget(QWidget *parent = Q_NULLPTR);
        void setType(PIETYPE t);
        void setLegendType(LEGENDTYPE t);
        void addItem(QString name, QColor color, float value);
        void removeItems(){ m_pieces.clear(); }

        void paintEvent(QPaintEvent * e);

    private:
        int m_margin_left, m_margin_top, m_margin_bottom, m_margin_right;
        double m_cX, m_cY, m_cW, m_cH, m_pW, m_lX, m_lY;
        int m_piece;
        bool m_shadows;
        QVector<pieceNC> m_pieces;
        int m_type, m_ltype;

        QPointF GetPoint(double angle, double R1 = 0, double R2 = 0);
        int GetQuater(double angle);
        double Angle360(double angle);
        void addPiece(QString name,Qt::GlobalColor,float Percentage);
        void addPiece(QString name,QColor, float Percentage);
        void setCords(double x, double y, double w, double h);
        void setLegendCords(double x, double y);
        void setShadows(bool ok = true);
        void draw(QPainter *painter);
        void drawLegend(QPainter *painter);
        int pieceCount();
        double palpha;

};

#endif // CHARTWIDGET_H/*
