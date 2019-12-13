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

#include "qrkgastro.h"
#include "qrkroomtablebuttons.h"
#include "preferences/qrksettings.h"
#include "database.h"
#include "utils/utils.h"
#include "3rdparty/ckvsoft/drag/dragflowwidget.h"
#include "3rdparty/ckvsoft/drag/dragpushbutton.h"

#include <QScrollBar>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

QrkRoomTableButtons::QrkRoomTableButtons(QWidget *parent) :
    QuickButtons(parent)
{
    setTopBoxHidden(true);
    setBoxName(BoxPosition::MIDDLE, tr("Räume"));
    setBoxName(BoxPosition::BOTTOM, tr("Tische"));
}

void QrkRoomTableButtons::quickTopButtons()
{

}

void QrkRoomTableButtons::quickMiddleButtons(int id)
{

    Q_UNUSED(id);
    if (!getSortOrderList(BoxPosition::BOTTOM).isEmpty()) {
        Database::updateSortorder("rooms", getSortOrderList(BoxPosition::BOTTOM, true));
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    bool ok = query.prepare("SELECT id, name, color, isHotel FROM rooms ORDER BY sortorder, name");

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.exec();

    DragFlowWidget *roomwidget = new DragFlowWidget(this);
    roomwidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    while (query.next()) {

        bool isHotel = query.value("isHotel").toBool();
        QString pbText = query.value(1).toString();
        DragPushButton *pb = new DragPushButton(roomwidget);
        pb->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        if (isHotel)
            pb->setIcon(QIcon(":/src/icons/sleeping-bed.png"));
        else
            pb->setIcon(QIcon(":/src/icons/restaurant-utensils-crossed.png"));

        pb->setIconSize(QSize(32,32));
        pb->setFixedSize(getQuickButtonSize());
        pb->setText(Utils::wordWrap(pbText, pb->width() - 8, pb->font()));

        QString backgroundColor = (query.value("color").toString() == "")? "#808080":query.value("color").toString();
        QString best_contrast = Utils::color_best_contrast(backgroundColor);

        pb->setStyleSheet(
                    "QToolButton {"
                    "margin: 3px;"
                    "border-color: black;"
                    "border-style: outset;"
                    "border-radius: 3px;"
                    "border-width: 1px;"
                    "color: " + best_contrast + ";"
                                                "background-color: " + backgroundColor + ";" // qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + backgroundColor + ", stop: 1 #0d5ca6);"
                    "}"
                    "QToolButton:disabled {"
                    "color: #dddddd;" // qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0d5ca6, stop: 1 " + backgroundColor + ");"
                    "background: transparent;"
                    "}"
                    "QToolButton:pressed {"
                    "border-color: green;"
                    "border-style: inset;"
                    "border-width: 2px;"
                    "}"
                    );
/*
                    "QToolButton:focus {"
                    "border-color: green;"
                    "border-style: inset;"
                    "border-width: 2px;"
                    "}"

*/
        pb->setId(query.value(0).toInt());
        roomwidget->addWidget(pb);
    }

    setMidWidget(roomwidget);
    connect(roomwidget, &DragFlowWidget::buttonClicked, this, &QrkRoomTableButtons::quickBottomButtons, Qt::DirectConnection);
    connect(roomwidget, &DragFlowWidget::orderChanged, this, &QrkRoomTableButtons::updateSortOrderGroups);
    if (m_currentRoomId == 0)
        emit quickBottomButtons(QRKGastro::getFirstRoomId());
    else
        emit quickBottomButtons(m_currentRoomId);

}

void QrkRoomTableButtons::quickBottomButtons(int id)
{

    setBoxName(BoxPosition::BOTTOM, tr("Tische: %1").arg(QRKGastro::getRoomName(id)));
    QBCMath totalSum(0);

    if (m_currentRoomId > 0) {
        DragPushButton *b = getRoomButton(m_currentRoomId);
        if (b)
            b->restoreBorderColor();
    }

    m_currentRoomId = id;
    DragPushButton *b = getRoomButton(m_currentRoomId);
    if (b)
        b->setBorderColor("green");

    if (!getSortOrderList(BoxPosition::MIDDLE).isEmpty()) {
        Database::updateSortorder("rooms", getSortOrderList(BoxPosition::MIDDLE, true));
    }

    if (!getSortOrderList(BoxPosition::BOTTOM).isEmpty()) {
        Database::updateSortorder("tables", getSortOrderList(BoxPosition::BOTTOM, true));
    }

    if (isMiddleBoxHidden()) {
        emit enableCategoriePushButton(true);
        emit showCategoriePushButton(true);
        setTopBoxHidden(true);
        setMiddleBoxHidden(false);
        setBottomBoxHidden(true);
    } else {
        emit enableCategoriePushButton(false);
        emit showCategoriePushButton(false);
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT color FROM rooms WHERE id=%1").arg(id));
    query.exec();
    QString bordercolor = "#808080";
    if (query.next())
        bordercolor = (query.value("color").toString() == "")?bordercolor: query.value("color").toString();

    bool ok = query.prepare("SELECT id, name, color FROM tables WHERE roomId=:roomid ORDER by sortorder, name");
    query.bindValue(":roomid", id);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.exec();

    DragFlowWidget *tablewidget = new DragFlowWidget(this);
    tablewidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    if (isMiddleBoxHidden()) {
        // backward button
        DragPushButton *pb = new DragPushButton(tablewidget);
        pb->setIcon(QIcon(":src/icons/backward.png"));
        pb->setIconSize(getQuickButtonSize() / 2);
        pb->setFixedSize(getQuickButtonSize());
        pb->setFixedButton(true);

        pb->setStyleSheet(
                    "QToolButton {"
                    "margin: 3px;"
                    "border-color: " + bordercolor + ";"
                                                     "border-style: outset;"
                                                     "border-radius: 3px;"
                                                     "border-width: 1px;"
                                                     "background-color: " + bordercolor + ";"
                                                                                          "}"
                    );

        connect(pb,&QToolButton::clicked,this,&QrkRoomTableButtons::backToMiddleButton);

        tablewidget->addWidget(pb);
    }

    while (query.next()) {

        DragPushButton *pb = new DragPushButton(tablewidget);
        int id = query.value("id").toInt();
        pb->setId(id);

        //        pb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        pb->setFixedSize(getQuickButtonSize());

        QString pbText = Utils::wordWrap(query.value("name").toString(), pb->width() - 8, pb->font());
        pbText = QString("%1")
                .arg(pbText);

        pb->setText(pbText);
        //        pb->setMinimumSize(pb->sizeHint());

        QString backgroundcolor = (query.value("color").toString() == "")?bordercolor: query.value("color").toString();

        QString best_contrast = Utils::color_best_contrast(backgroundcolor);

        pb->setStyleSheet(
                    "QToolButton {"
                    "margin: 3px;"
                    "border-color: " + bordercolor + ";"
                    "border-style: outset;"
                    "border-radius: 3px;"
                    "border-width: 1px;"
                    "color: " + best_contrast + ";"
                    "background-color: " + backgroundcolor + ";"
                    "}"
                    "QToolButton:disabled {"
                    "color: #dddddd;"
                    "background: transparent;"
                    "}"
                    "QToolButton:pressed {"
                    "border-color: green;"
                    "border-style: inset;"
                    "border-width: 2px;"
                    "}"
                    );

        pb->setFlashEnabled(false);
        QBCMath sum(0);
        double sumDouble = QRKGastro::getOrderSum(id).toDouble();
        totalSum += sumDouble;
        if(sumDouble != 0.00) {
            pb->setButtonColor("green");
            sum = sumDouble;
            if (QRKGastro::isOrderNotServed(id))
                pb->setFlashEnabled(true);
            else
                pb->setFlashEnabled(false);
        }
        sum.round(2);
        pb->setPriceText(sum.toLocale() + " " + QLocale().currencySymbol());
        tablewidget->addWidget(pb);
    }

    totalSum.round(2);
    double dts = totalSum.toDouble();

    if (b != Q_NULLPTR) {
        if ( dts != 0.00) {
            b->setFlashEnabled(true);
        } else {
            b->setFlashEnabled(false);
        }
    }

    setBotWidget(tablewidget);
    connect(tablewidget, &DragFlowWidget::buttonClicked, this, &QrkRoomTableButtons::tableOrder, Qt::QueuedConnection);
    connect(tablewidget, &DragFlowWidget::orderChanged, this, &QrkRoomTableButtons::updateSortOrderProducts);
}

void QrkRoomTableButtons::backToTopButton(bool clicked)
{
    Q_UNUSED(clicked);
}

void QrkRoomTableButtons::backToMiddleButton(bool clicked)
{
    Q_UNUSED(clicked);

    if (!getSortOrderList(BoxPosition::BOTTOM).isEmpty()) {
        Database::updateSortorder("tables", getSortOrderList(BoxPosition::BOTTOM, true));
    }

    setMiddleBoxHidden(false);
    setBottomBoxHidden(true);
}

DragPushButton *QrkRoomTableButtons::getTableButton(int id)
{
    DragFlowWidget *w = getBotWidget();
    if (w)
        return w->getDragPushButton(id);

    return new DragPushButton(this);
}

DragPushButton *QrkRoomTableButtons::getRoomButton(int id)
{
    DragFlowWidget *w = getMidWidget();
    if (w)
        return w->getDragPushButton(id);

    return new DragPushButton(this);
}

void QrkRoomTableButtons::readSettings()
{
    QrkSettings settings;

    settings.beginGroup("Gastro");
    setStretchFactor(settings.value("roomTableStretchFactor", QSize(1, 4)).toSize());
    setDirection(settings.value("roomTableDirection", 1).toInt());
    setMiddleBoxHidden(settings.value("roomsHidden", false).toBool());
    setQuickButtonSize(settings.value("roomTableButtonSize", QSize(150, 80)).toSize());
    settings.endGroup();
}
