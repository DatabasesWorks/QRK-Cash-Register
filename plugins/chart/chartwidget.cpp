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

#include "chartwidget.h"

#include <QPainter>
#include <QVector>
#include <QLinearGradient>
#include <math.h>

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent)
{
    m_type = Dpie;
    m_ltype = Vertical;
    m_cX = 0;
    m_cY = 0;
    m_cW = 100;
    m_cH = 100;
    m_lX = m_cX + m_cW + 20;
    m_lY = m_cY;
    m_shadows = true;

    m_margin_left = 16;
    m_margin_top = 16;
    m_margin_bottom = 16;
    m_margin_right = 16;
}

void ChartWidget::paintEvent(QPaintEvent * e)
{
    QWidget::paintEvent(e);
    if(!pieceCount()) return ;
    QPainter painter;
    painter.begin(this);
    int w = (width() - m_margin_left - m_margin_right);
    int h = (height() - m_margin_top - m_margin_bottom);
    int size = (w < h)? w: h;
    setCords(m_margin_left, m_margin_top, size, size);

    draw(&painter);
    drawLegend(&painter);
}

void ChartWidget::addItem(QString name, QColor color, float value)
{
    addPiece(name, color, value);
}

void ChartWidget::addPiece(QString name,Qt::GlobalColor color,float Percentage)
{
    m_piece++;

    pieceNC piece;
    piece.addName(name);
    piece.setColor(color);
    piece.setPerc(Percentage);
    m_pieces.append(piece);
}

void ChartWidget::addPiece(QString name, QColor color, float Percentage)
{
    m_piece++;
    pieceNC piece;
    piece.addName(name);
    piece.setColor(color);
    piece.setPerc(Percentage);
    m_pieces.append(piece);

}

void ChartWidget::setCords(double x, double y, double w, double h)
{
    m_cX = x;
    m_cY = y;
    m_cW = w;
    m_cH = h;
    m_lX = m_cX + m_cW+20;
    m_lY = m_cY;
}

void ChartWidget::setLegendCords(double x, double y)
{
    m_lX = x;
    m_lY = y;
}

void ChartWidget::setType(PIETYPE t)
{
    m_type = t;
}

void ChartWidget::setLegendType(LEGENDTYPE t)
{
    if (t != None) {
        m_margin_right += 150;
        m_margin_bottom += 100;
    }

    m_ltype = t;
}

void ChartWidget::setShadows(bool shadow)
{
    m_shadows = shadow;
}

void ChartWidget::draw(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    if (m_type==Pie)
    {
        m_pW = 0;
        double pdegree = 0;

        //Options
        QLinearGradient gradient(m_cX + 0.5 * m_cW, m_cY, m_cX + 0.5 * m_cW, m_cY + m_cH * 2.5);
        gradient.setColorAt(1,Qt::black);


        //Draw
        //pdegree = (360/100)*pieces[i].pPerc;
        if (m_shadows)
        {
            double sumangle = 0;
            for (int i=0;i<m_pieces.size();i++)
            {
                sumangle += 3.6 * m_pieces[i].pPerc;
            }
            painter->setBrush(Qt::darkGray);
            painter->drawPie(m_cX, m_cY + m_pW + 5, m_cW, m_cH, palpha * 16, sumangle * 16);
        }

        QPen pen;
        pen.setWidth(2);

        for (int i=0; i < m_pieces.size(); i++)
        {
            gradient.setColorAt(0,m_pieces[i].rgbColor);
            painter->setBrush(gradient);
            pen.setColor(m_pieces[i].rgbColor);
            painter->setPen(pen);
            pdegree = 3.6 * m_pieces[i].pPerc;
            painter->drawPie(m_cX, m_cY, m_cW, m_cH, palpha * 16, pdegree * 16);
            palpha += pdegree;
        }
    }
    else if (m_type==Dpie)
    {
        m_pW = 50;
        double pdegree = 0;
        QPointF p;

        QLinearGradient gradient(m_cX - 0.5 * m_cW, m_cY + m_cH / 2, m_cX +1.5 * m_cW, m_cY + m_cH / 2);
        gradient.setColorAt(0,Qt::black);
        gradient.setColorAt(1,Qt::white);
        QLinearGradient gradient_side(m_cX, m_cY + m_cH, m_cX + m_cW, m_cY + m_cH);
        gradient_side.setColorAt(0,Qt::black);

        double sumangle = 0;
        for (int i = 0; i < m_pieces.size(); i++)
        {
            sumangle += 3.6 * m_pieces[i].pPerc;
        }
        if (m_shadows)
        {
            painter->setBrush(Qt::darkGray);
            painter->drawPie(m_cX, m_cY + m_pW + 5, m_cW, m_cH, palpha * 16, sumangle * 16);
        }
        int q = GetQuater(palpha+sumangle);

        if (q ==2 || q==3)
        {
            QPointF p = GetPoint(palpha+sumangle);
            QPointF points[4] =
            {
                QPointF(p.x(),p.y()),
                QPointF(p.x(), p.y() + m_pW),
                QPointF(m_cX + m_cW / 2, m_cY + m_cH / 2 + m_pW),
                QPointF(m_cX + m_cW / 2, m_cY + m_cH / 2)
            };
            gradient_side.setColorAt(1, m_pieces[m_pieces.size()-1].rgbColor);
            painter->setBrush(gradient_side);
            painter->drawPolygon(points,4);
        }
        p = GetPoint(palpha);
        q = GetQuater(palpha);
        if (q ==1 || q==4)
        {
            QPointF points[4] =
            {
                QPointF(p.x(), p.y()),
                QPointF(p.x(), p.y() + m_pW),
                QPointF(m_cX + m_cW / 2, m_cY + m_cH / 2 + m_pW),
                QPointF(m_cX + m_cW / 2, m_cY + m_cH / 2)
            };
            gradient_side.setColorAt(1, m_pieces[0].rgbColor);
            painter->setBrush(gradient_side);
            painter->drawPolygon(points,4);
        }

        for (int i = 0; i < m_pieces.size(); i++)
        {
            gradient.setColorAt(0.5, m_pieces[i].rgbColor);
            painter->setBrush(gradient);
            pdegree = 3.6 * m_pieces[i].pPerc;
            painter->drawPie(m_cX, m_cY, m_cW, m_cH, palpha * 16, pdegree * 16);

            double a_ = Angle360(palpha);
            int q_ = GetQuater(palpha);

            palpha += pdegree;

            double a = Angle360(palpha);
            int q = GetQuater(palpha);

            QPainterPath path;
            p = GetPoint(palpha);

            if((q == 3 || q == 4) && (q_ == 3 || q_ == 4))
            {
                // 1)
                if (a>a_)
                {
                    QPointF p_old = GetPoint(palpha-pdegree);
                    path.moveTo(p_old.x()-1,p_old.y());
                    path.arcTo(m_cX, m_cY, m_cW, m_cH, palpha-pdegree, pdegree);
                    path.lineTo(p.x(), p.y() + m_pW);
                    path.arcTo(m_cX, m_cY + m_pW, m_cW, m_cH, palpha, -pdegree);
                }
                // 2)
                else
                {
                    path.moveTo(m_cX, m_cY + m_cH / 2);
                    path.arcTo(m_cX, m_cY, m_cW, m_cH, 180, Angle360(palpha) - 180);
                    path.lineTo(p.x(), p.y() + m_pW);
                    path.arcTo(m_cX, m_cY + m_pW, m_cW, m_cH, Angle360(palpha), -Angle360(palpha) + 180);
                    path.lineTo(m_cX, m_cY + m_cH / 2);

                    path.moveTo(p.x(),p.y());
                    path.arcTo(m_cX, m_cY, m_cW, m_cH, palpha - pdegree, 360 - Angle360(palpha-pdegree));
                    path.lineTo(m_cX + m_cW, m_cY + m_cH / 2 + m_pW);
                    path.arcTo(m_cX, m_cY + m_pW, m_cW, m_cH, 0, -360+Angle360(palpha-pdegree));
                }

            }
            // 3)
            else if((q == 3 || q == 4) && (q_ == 1 || q_ == 2) && a>a_ )
            {
                path.moveTo(m_cX, m_cY + m_cH / 2);
                path.arcTo(m_cX, m_cY, m_cW, m_cH, 180, Angle360(palpha)-180);
                path.lineTo(p.x(), p.y() + m_pW);
                path.arcTo(m_cX, m_cY + m_pW, m_cW, m_cH, Angle360(palpha), -Angle360(palpha) + 180);
                path.lineTo(m_cX, m_cY + m_cH / 2);
            }
            // 4)
            else if((q == 1 || q == 2) && (q_ == 3 || q_ == 4) && a<a_)
            {
                p = GetPoint(palpha-pdegree);
                path.moveTo(p.x(),p.y());
                path.arcTo(m_cX, m_cY, m_cW, m_cH, palpha - pdegree, 360 - Angle360(palpha-pdegree));
                path.lineTo(m_cX + m_cW, m_cY + m_cH / 2 + m_pW);
                path.arcTo(m_cX, m_cY + m_pW, m_cW, m_cH, 0, -360 + Angle360(palpha-pdegree));
            }
            // 5)
            else if((q ==1 || q==2) && (q_==1 || q_==2) && a<a_)
            {
                path.moveTo(m_cX, m_cY + m_cH / 2);
                path.arcTo(m_cX, m_cY, m_cW, m_cH, 180, 180);
                path.lineTo(m_cX + m_cW, m_cY + m_cH / 2 + m_pW);
                path.arcTo(m_cX, m_cY + m_pW, m_cW, m_cH, 0, -180);
                path.lineTo(m_cX, m_cY + m_cH / 2);
            }
            if (!path.isEmpty())
            {
                gradient_side.setColorAt(1, m_pieces[i].rgbColor);
                painter->setBrush(gradient_side);
                painter->drawPath(path);
            }
        }
    }
    else if (m_type==Histogramm)
    {
        double pDist = 15;
        // double pW = (m_cW - (m_pieces.size()) * pDist) / m_pieces.size();

        QLinearGradient gradient(m_cX + m_cW / 2, m_cY, m_cX + m_cW / 2, m_cY + m_cH);
        gradient.setColorAt(0,Qt::black);
        QPen pen;
        pen.setWidth(3);

        for (int i = 0; i < m_pieces.size(); i++)
        {
            if (m_shadows)
            {
                painter->setPen(Qt::NoPen);
                painter->setBrush(Qt::darkGray);
                painter->drawRect(m_cX + pDist + i * (m_pW + pDist) - pDist / 2, m_cY + m_cH-1, m_pW, -m_cH / 100 * m_pieces[i].pPerc + pDist / 2 - 5);
            }
            gradient.setColorAt(1, m_pieces[i].rgbColor);
            painter->setBrush(gradient);
            pen.setColor(m_pieces[i].rgbColor);
            painter->setPen(pen);
            painter->drawRect(m_cX + pDist + i * (m_pW + pDist), m_cY + m_cH, m_pW, - m_cH / 100 * m_pieces[i].pPerc - 5);
            QString label = QString::number(m_pieces[i].pPerc) + "%";
            painter->setPen(Qt::SolidLine);
            painter->drawText(m_cX + pDist + i * (m_pW + pDist) + m_pW / 2 - painter->fontMetrics().width(label) / 2, m_cY + m_cH -m_cH / 100 * m_pieces[i].pPerc-painter->fontMetrics().height() / 2, label);
        }
        painter->setPen(Qt::SolidLine);
        for (int i=1;i<10;i++)
        {
            painter->drawLine(m_cX - 3, m_cY + m_cH / 10 * i, m_cX + 3, m_cY + m_cH / 10 * i);    //äåëåíèÿ ïî îñè Y
            //painter->drawText(cX-20,cY+cH/10*i,QString::number((10-i)*10)+"%");
        }
        painter->drawLine(m_cX, m_cY + m_cH, m_cX, m_cY);         //îñü Y
        painter->drawLine(m_cX, m_cY, m_cX + 4, m_cY + 10);       //ñòðåëêè
        painter->drawLine(m_cX, m_cY, m_cX - 4, m_cY + 10);
        painter->drawLine(m_cX, m_cY + m_cH, m_cX + m_cW, m_cY + m_cH);   //îñü Õ

    }
}

void ChartWidget::drawLegend(QPainter *painter)
{
    //double ptext = 25;
    double angle = palpha;
    painter->setPen(Qt::SolidLine);

    switch(m_ltype)
    {
    /*case Nightcharts::Horizontal:
    {
        int dist = 5;
        painter->setBrush(Qt::white);
        float x = cX;
        float y = cY+cH+20+dist;
        //painter->drawRoundRect(cX+cW+20,cY,dist*2+200,pieces.size()*(fontmetr.height()+2*dist)+dist,15,15);
        for (int i=0;i<pieces.size();i++)
        {
            painter->setBrush(pieces[i].rgbColor);
            x += fontmetr.height()+2*dist;
            if (i%3 == 0)
            {
                x = cX;
                y += dist+fontmetr.height();
            }
            painter->drawRect(x,y,fontmetr.height(),fontmetr.height());
            QString label = pieces[i].pname + " - " + QString::number(pieces[i].pPerc)+"%";
            painter->drawText(x+fontmetr.height()+dist,y+fontmetr.height()/2+dist,label);
            x += fontmetr.width(label);
        }
        break;
    }*/
    case Vertical:
    {
        int dist = 5;
        painter->setBrush(Qt::white);
        //painter->drawRoundRect(cX+cW+20,cY,dist*2+200,pieces.size()*(painter->fontMetrics().height()+2*dist)+dist,15,15);
        for (int i = m_pieces.size()-1;i>=0;i--)
        {
            painter->setBrush(m_pieces[i].rgbColor);
            float x = m_lX + dist;
            float y = m_lY + dist + i * (painter->fontMetrics().height() + 2 * dist);
            painter->drawRect(x, y, painter->fontMetrics().height(),painter->fontMetrics().height());
            painter->drawText(x + painter->fontMetrics().height() + dist, y + painter->fontMetrics().height() / 2 + dist, m_pieces[i].pname + " - " + QString::number(m_pieces[i].pPerc) + "%");
        }
        break;
    }
    case None:
    {
        break;
    }

    case Round:
        for (int i = m_pieces.size() - 1; i >= 0; i--)
        {
            float len = 100;
            double pdegree = 3.6 * m_pieces[i].pPerc;
            angle -= pdegree / 2;
            QPointF p = GetPoint(angle);
            QPointF p_ = GetPoint(angle, m_cW + len, m_cH + len);
            int q = GetQuater(angle);
            if (q == 3 || q == 4)
            {
                p.setY(p.y() + m_pW / 2);
                p_.setY(p_.y() + m_pW / 2);
            }
            painter->drawLine(p.x(),p.y(),p_.x(),p_.y());
            QString label = m_pieces[i].pname + " - " + QString::number(m_pieces[i].pPerc) + "%";
            float recW = painter->fontMetrics().width(label)+10;
            float recH = painter->fontMetrics().height()+10;
            p_.setX(p_.x()-recW/2 + recW/2*cos(angle*M_PI/180));
            p_.setY(p_.y()+recH/2 + recH/2*sin(angle*M_PI/180));
            painter->setBrush(Qt::white);
            painter->drawRoundRect(p_.x() ,p_.y(), recW, -recH);
            painter->drawText(p_.x()+5, p_.y()-recH/2+5, label);
            angle -= pdegree/2;
        }
        break;
    }
}

QPointF ChartWidget::GetPoint(double angle, double R1, double R2)
{
    if (R1 == 0 && R2 == 0)
    {
        R1 = m_cW;
        R2 = m_cH;
    }
    QPointF point;
    double x = R1 / 2 * cos(angle * M_PI/180);
    x += m_cW / 2 + m_cX;
    double y = -R2 / 2 * sin(angle * M_PI/180);
    y += m_cH / 2 + m_cY;
    point.setX(x);
    point.setY(y);
    return point;
}

int ChartWidget::GetQuater(double angle)
{
    angle = Angle360(angle);

    if(angle>=0 && angle<90)
        return 1;
    if(angle>=90 && angle<180)
        return 2;
    if(angle>=180 && angle<270)
        return 3;
    if(angle>=270 && angle<360)
        return 4;

    return 0;
}

double ChartWidget::Angle360(double angle)
{
    int i = (int)angle;
    double delta = angle - i;
    return (i%360 + delta);
}

pieceNC::pieceNC()
{
}
void pieceNC::addName(QString name)
{
    pname = name;
}
void pieceNC::setColor(Qt::GlobalColor color)
{
    rgbColor = color;
}
void pieceNC::setColor(QColor color)
{
    rgbColor = color;
}

void pieceNC::setPerc(float Percentage)
{
    pPerc = Percentage;
}

int ChartWidget::pieceCount()
{
    return m_pieces.count();
}
