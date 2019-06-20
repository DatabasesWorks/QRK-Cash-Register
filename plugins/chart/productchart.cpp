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

#include "productchart.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "qrkpushbutton.h"
#include "database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QTableView>
#include <QSplitter>
#include <QComboBox>
#include <QLabel>
#include <QHeaderView>

ProductChart::ProductChart(QWidget *parent) : QDialog(parent)
{
    setupModel();
    setupViews();

    loadData();

    setWindowTitle(tr("Chart"));
    resize(750, 400);
}

void ProductChart::setupModel()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery q(dbc);
    q.prepare("SELECT COUNT(id) FROM products WHERE products.groupid > 1");
    int rows = 0;
    if (q.next())
        rows = q.value(0).toInt();

    model = new QStandardItemModel(rows, 3, this);
    model->setHeaderData(0, Qt::Horizontal, tr("Produktname"));
    model->setHeaderData(1, Qt::Horizontal, tr("verkauft"));
    model->setHeaderData(2, Qt::Horizontal, "%");
}

void ProductChart::setupViews()
{

    QVBoxLayout *vlayout =  new QVBoxLayout;
    QSplitter *splitter = new QSplitter;
    QTableView *table = new QTableView;

    m_chart = new ChartWidget;
    m_chart->setType(ChartWidget::Pie);
    m_chart->setLegendType(ChartWidget::None);

    splitter->addWidget(table);
    splitter->addWidget(m_chart);
    splitter->setSizes(QList<int>({150, 100}));

    table->setModel(model);

    QHBoxLayout *comboLayout = new QHBoxLayout;
    m_combo = new QComboBox;
    m_combo->addItem("10", 10);
    m_combo->addItem("20", 20);
    m_combo->addItem("50", 50);
    m_combo->addItem(tr("alle"), -1);
    m_combo->setCurrentText("10");
    QLabel *comboLabel = new QLabel;
    comboLabel->setText(tr("Anzeigen der Top"));
    QSpacerItem* comboSpacer = new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding );
    comboLayout->addWidget(comboLabel);
    comboLayout->addWidget(m_combo);
    comboLayout->addItem(comboSpacer);

    QrkPushButton *pushButton = new QrkPushButton;
    pushButton->setMinimumHeight(60);
    pushButton->setMinimumWidth(0);

    QIcon icon = QIcon(":src/icons/ok.png");
    QSize size = QSize(24,24);
    pushButton->setIcon(icon);
    pushButton->setIconSize(size);
    pushButton->setText(tr("OK"));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QSpacerItem* spacer = new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding );
    buttonLayout->addItem(spacer);
    buttonLayout->addWidget(pushButton);

    QItemSelectionModel *selectionModel = new QItemSelectionModel(model);
    table->setSelectionModel(selectionModel);
    table->setSortingEnabled(true);
    table->sortByColumn(1,Qt::DescendingOrder);
    QHeaderView *headerView = table->horizontalHeader();
    headerView->setSectionResizeMode(0, QHeaderView::Stretch);

    QFrame *lineH1 = new QFrame;
    lineH1->setFrameShape(QFrame::HLine);
    QFrame *lineH2 = new QFrame;
    lineH2->setFrameShape(QFrame::HLine);

    vlayout->addLayout(comboLayout);
    vlayout->addWidget(lineH1);
    vlayout->addWidget(splitter);
    vlayout->addWidget(lineH2);
    vlayout->addLayout(buttonLayout);
    vlayout->setStretch(2,1);
    setLayout(vlayout);

    connect(pushButton, &QPushButton::clicked, this, &ProductChart::close);
    connect(m_combo, &QComboBox::currentTextChanged, this, &ProductChart::comboBoxChanged);
}

void ProductChart::loadData(SORTORDER order)
{

    QSqlDatabase dbc = Database::database();
    QSqlQuery q(dbc);

    q.prepare("SELECT sum(sold) as sold, sum(sold * gross) as gross FROM products WHERE products.groupid > 1");
    q.exec();
    q.next();
    QBCMath whole(q.value("sold").toDouble());
    if (order == GROSS)
        whole = q.value("gross").toDouble();
    whole.round(2);

    QBCMath current(0.0);

    if (order == SOLD)
        q.prepare("SELECT name, sold FROM products  WHERE products.groupid > 1 ORDER BY sold DESC LIMIT :limit");
    else
        q.prepare("SELECT name, sold * gross as sumgross FROM products  WHERE products.groupid > 1 ORDER BY sumgross DESC LIMIT :limit");

    q.bindValue(":limit", m_maxview);
    q.exec();

    model->removeRows(0, model->rowCount(QModelIndex()), QModelIndex());

    int size = QColor::colorNames().size();
    int row = 0;
    int color = 0;
    while (q.next()) {
        model->insertRows(row, 1, QModelIndex());

        QString name = q.value("name").toString();
        QBCMath part;
        if (order == SOLD) {
            part = q.value("sold").toDouble();
        } else {
            part = q.value("sumgross").toDouble();
        }

        part.round(2);

        current += part;
        QBCMath percentage((part / whole) * 100);
        percentage.round(2);

        model->setData(model->index(row, 0, QModelIndex()), name);
        model->setData(model->index(row, 1, QModelIndex()), part.toString());
        model->setData(model->index(row, 2, QModelIndex()), percentage.toString().rightJustified(5, '0') + " %");
        model->setData(model->index(row, 1, QModelIndex()),Qt::AlignRight,Qt::TextAlignmentRole);
        model->setData(model->index(row, 2, QModelIndex()),Qt::AlignRight,Qt::TextAlignmentRole);

        model->setData(model->index(row, 0, QModelIndex()),
                       QColor(QColor::colorNames().at(color)), Qt::DecorationRole);

        m_chart->addItem(name, QColor::colorNames().at(color), percentage.toDouble());
        row++;
        color++;
        if (color+2 > size) color = 0;
    }

    if (whole > 0) {
        QBCMath percentage(((whole - current) / whole) * 100);
        QBCMath part(whole - current);
        part.round(2);
        percentage.round(2);
        QString name = tr("Rest");
        m_chart->addItem(name, QColor::colorNames().at(color+1), percentage.toDouble());
        if (percentage.toDouble() > 0.00) {
            model->insertRows(row, 1, QModelIndex());
            model->setData(model->index(row, 0, QModelIndex()), name);
            model->setData(model->index(row, 1, QModelIndex()), part.toString());
            model->setData(model->index(row, 2, QModelIndex()), percentage.toString().rightJustified(5, '0') + " %");
            model->setData(model->index(row, 1, QModelIndex()),Qt::AlignRight,Qt::TextAlignmentRole);
            model->setData(model->index(row, 2, QModelIndex()),Qt::AlignRight,Qt::TextAlignmentRole);

            model->setData(model->index(row, 0, QModelIndex()),
                           QColor(QColor::colorNames().at(color+1)), Qt::DecorationRole);
        }
    }
}

void ProductChart::comboBoxChanged(QString)
{
    m_maxview = m_combo->currentData().toInt();
    m_chart->removeItems();
    loadData();
    m_chart->repaint();
}
