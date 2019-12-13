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

#include "3rdparty/ckvsoft/drag/dragpushbutton.h"
#include "qrkquickbuttons.h"
#include "preferences/qrksettings.h"
#include "database.h"
#include "utils/utils.h"
#include "drag/dragflowwidget.h"

#include <QScrollBar>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

QrkQuickButtons::QrkQuickButtons(QWidget *parent) :
    QuickButtons(parent)
{
    setBoxName(BoxPosition::TOP, tr("Kategorien"));
    setBoxName(BoxPosition::MIDDLE, tr("Warengruppen"));
    setBoxName(BoxPosition::BOTTOM, tr("Artikel"));
}

void QrkQuickButtons::quickTopButtons()
{
    if (!getSortOrderList(BoxPosition::BOTTOM).isEmpty()) {
        Database::updateSortorder("products", getSortOrderList(BoxPosition::BOTTOM, true));
    }

    if (!m_usecategories)
        return;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    bool ok = query.prepare("SELECT id, name, color FROM categories ORDER BY sortorder, name");

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.exec();

    DragFlowWidget *widget = new DragFlowWidget(this);
    widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    while (query.next()) {

        QString pbText = query.value(1).toString();
        DragPushButton *pb = new DragPushButton(widget);
        pb->setFixedSize(getQuickButtonSize());
        pb->setText(Utils::wordWrap(pbText, pb->width() - 8, pb->font()));

        QString backgroundColor = (query.value(2).toString() == "")? "#808080":query.value(2).toString();
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
                    "QToolButton:focus {"
                    "border-color: green;"
                    "border-style: inset;"
                    "border-width: 2px;"
                    "}"
                    );

        pb->setId(query.value(0).toInt());
        widget->addWidget(pb);
    }

    setTopWidget(widget);
    connect(widget, &DragFlowWidget::buttonClicked, this, &QrkQuickButtons::quickMiddleButtons);
    connect(widget, &DragFlowWidget::orderChanged, this, &QrkQuickButtons::updateSortOrderGroups);
}

void QrkQuickButtons::quickMiddleButtons(int id)
{

    if (m_usecategories) {
        if (m_currentCategorieId > 0) {
            DragPushButton *pb = getCategorieButton(m_currentCategorieId);
            if (pb != Q_NULLPTR)
                pb->restoreBorderColor();
        }
        m_currentCategorieId = id;
        DragPushButton *pb = getCategorieButton(m_currentCategorieId);
        if (pb != Q_NULLPTR)
            pb->setBorderColor("green");
    }

    if (isBottomBoxHidden()) {
        setTopBoxHidden(true);
        setMiddleBoxHidden(false);
    }

    if (!getSortOrderList(BoxPosition::TOP).isEmpty()) {
        Database::updateSortorder("categories", getSortOrderList(BoxPosition::TOP, true));
    }

    if (!getSortOrderList(BoxPosition::MIDDLE).isEmpty()) {
        Database::updateSortorder("groups", getSortOrderList(BoxPosition::MIDDLE, true));
    }

    if (!getSortOrderList(BoxPosition::BOTTOM).isEmpty()) {
        Database::updateSortorder("products", getSortOrderList(BoxPosition::BOTTOM, true));
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare(QString("SELECT color FROM categories WHERE id=%1").arg(id));
    query.exec();
    QString bordercolor = "#808080";
    if (query.next())
        bordercolor = (query.value("color").toString() == "")?bordercolor: query.value("color").toString();

    bool ok;

    if (m_usecategories) {
        ok = query.prepare("SELECT id, name, color FROM groups WHERE visible=1 AND categoryId=:categoryid ORDER BY sortorder, name");
        query.bindValue(":categoryid",id);
    } else {
        ok = query.prepare("SELECT id, name, color FROM groups WHERE visible=1 ORDER BY sortorder, name");
    }

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.exec();

    DragFlowWidget *widget = new DragFlowWidget(this);
    widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    if (m_groupbuttonsettings && m_usecategories && m_categoriesbuttonsettings) {
        // backward button
        DragPushButton *pb = new DragPushButton();
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
                    "QToolButton:pressed {"
                    "border-color: green;"
                    "border-style: inset;"
                    "border-width: 2px;"
                    "}"
                    );

        connect(pb,&QToolButton::clicked,this,&QrkQuickButtons::backToTopButton);

        widget->addWidget(pb);
    }

    while (query.next()) {

        QString pbText = query.value(1).toString();
        DragPushButton *pb = new DragPushButton(widget);
        pb->setFixedSize(getQuickButtonSize());
        pb->setText(Utils::wordWrap(pbText, pb->width() - 8, pb->font()));

        QString backgroundColor = (query.value(2).toString() == "")? bordercolor:query.value(2).toString();
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
                    );

        pb->setId(query.value(0).toInt());
        widget->addWidget(pb);
    }

    setMidWidget(widget);
    connect(widget, &DragFlowWidget::buttonClicked, this, &QrkQuickButtons::quickBottomButtons);
    connect(widget, &DragFlowWidget::orderChanged, this, &QrkQuickButtons::updateSortOrderGroups);
}

void QrkQuickButtons::quickBottomButtons(int id)
{
    if (id == 0)
        return;

    if (m_currentGroupId > 0)
        getGroupButton(m_currentGroupId)->restoreBorderColor();
    m_currentGroupId = id;
    getGroupButton(m_currentGroupId)->setBorderColor("green");

    if (!getSortOrderList(BoxPosition::MIDDLE).isEmpty()) {
        Database::updateSortorder("groups", getSortOrderList(BoxPosition::MIDDLE, true));
    }

    if (!getSortOrderList(BoxPosition::BOTTOM).isEmpty()) {
        Database::updateSortorder("products", getSortOrderList(BoxPosition::BOTTOM, true));
    }

    if (m_groupbuttonsettings && id > 0) {
        emit enableCategoriePushButton(true);
        emit showCategoriePushButton(true);
        setTopBoxHidden(true);
        setMiddleBoxHidden(true);
        setBottomBoxHidden(false);
    } else {
        emit enableCategoriePushButton(false);
        emit showCategoriePushButton(false);
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT color FROM groups WHERE id=%1").arg(id));
    query.exec();
    QString bordercolor = "#808080";
    if (query.next())
        bordercolor = (query.value("color").toString() == "")?bordercolor: query.value("color").toString();

    bool ok = query.prepare("SELECT id, name, gross, color FROM products WHERE groupid=:groupid AND visible=1 ORDER by sortorder, name");
    query.bindValue(":groupid", id);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.exec();

    DragFlowWidget *widget = new DragFlowWidget(this);
    widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    if (m_groupbuttonsettings) {
        // backward button
        DragPushButton *pb = new DragPushButton();
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
                    "QToolButton:pressed {"
                    "border-color: green;"
                    "border-style: inset;"
                    "border-width: 2px;"
                    "}"
                    );

        connect(pb,&QToolButton::clicked,this,&QrkQuickButtons::backToMiddleButton);

        widget->addWidget(pb);
    }

    while (query.next()) {
        DragPushButton *pb = new DragPushButton();
        pb->setId(query.value("id").toInt());

        //        pb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        pb->setFixedSize(getQuickButtonSize());

        QString pbText = Utils::wordWrap(query.value("name").toString(), pb->width() - 8, pb->font());
        pbText = QString("%1\n %2 %3")
                .arg(pbText)
                .arg(QLocale().toString(query.value("gross").toDouble(),'f',2))
                .arg(Database::getShortCurrency());

        pb->setText(pbText);
        //        pb->setMinimumSize(pb->sizeHint());

        QString backgroundcolor = (query.value("color").toString() == "")?bordercolor: query.value(3).toString();

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

        widget->addWidget(pb);
    }

    setBotWidget(widget);
    connect(widget, &DragFlowWidget::buttonClicked, this, &QrkQuickButtons::addProductToOrderList, Qt::QueuedConnection);
    connect(widget, &DragFlowWidget::orderChanged, this, &QrkQuickButtons::updateSortOrderProducts);
}

void QrkQuickButtons::backToTopButton(bool clicked)
{
    Q_UNUSED(clicked);

    if (!getSortOrderList(BoxPosition::MIDDLE).isEmpty()) {
        Database::updateSortorder("groups", getSortOrderList(BoxPosition::MIDDLE, true));
    }

    setMiddleBoxHidden(true);
    setBottomBoxHidden(true);
    setTopBoxHidden(false);
}

void QrkQuickButtons::backToMiddleButton(bool clicked)
{
    Q_UNUSED(clicked);

    if (!getSortOrderList(BoxPosition::BOTTOM).isEmpty()) {
        Database::updateSortorder("products", getSortOrderList(BoxPosition::BOTTOM, true));
    }

    setTopBoxHidden(true);
    setMiddleBoxHidden(false);
    setBottomBoxHidden(true);
}

DragPushButton *QrkQuickButtons::getCategorieButton(int id)
{
    DragFlowWidget *w = getTopWidget();
    if (w != Q_NULLPTR)
        return w->getDragPushButton(id);

    return new DragPushButton(this);
}

DragPushButton *QrkQuickButtons::getGroupButton(int id)
{
    DragFlowWidget *w = getMidWidget();
    if (w)
        return w->getDragPushButton(id);

    return new DragPushButton(this);
}

void QrkQuickButtons::readSettings()
{
    QrkSettings settings;

    setStretchFactor(settings.value("productGroupStretchFactor", QSize(1, 1)).toSize());
    setDirection(settings.value("productGroupDirection", 0).toInt());
    m_groupbuttonsettings = settings.value("productGroupHidden", true).toBool();
    m_categoriesbuttonsettings = settings.value("productCategoriesHidden", true).toBool();
    m_usecategories = settings.value("useProductCategories", false).toBool();
    if (m_groupbuttonsettings) {
        setTopBoxHidden(true);
        setBottomBoxHidden(true);
    } else {
        bool hidden = !(m_usecategories && !m_categoriesbuttonsettings);
        setTopBoxHidden(hidden);
        setBottomBoxHidden(false);
    }

    setQuickButtonSize(settings.value("quickButtonSize", QSize(150, 80)).toSize());
    emit quickTopButtons();
    emit quickMiddleButtons();
    emit quickBottomButtons(0);
}
