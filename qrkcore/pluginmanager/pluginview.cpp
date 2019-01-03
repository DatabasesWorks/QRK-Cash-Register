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

#include "pluginview.h"
#include "pluginmanager/treemodel.h"
#include "pluginmanager/Interfaces/plugininterface.h"

#include "pluginmanager.h"
#include "ui_pluginview.h"

PluginView::PluginView(QWidget *parent) :
    QDialog(parent, Qt::WindowCloseButtonHint),
    ui(new Ui::PluginView)
{
    ui->setupUi(this);
    setCloseButtonVisible(false);

    setWindowTitle(tr("Plugins ..."));

    m_model = new TreeModel(this);
    ui->treeView->setModel(m_model);
    ui->treeView->setWindowTitle(tr("Plugins ..."));
    ui->treeView->setColumnHidden(3, true);

    connect(ui->treeView, &QTreeView::doubleClicked, this, &PluginView::itemDoubleClicked);
    connect(ui->settingsButton, &QPushButton::clicked, this, &PluginView::settingsButtonClicked);
    connect(ui->closeButton, &QPushButton::clicked, this, &PluginView::close);
}

PluginView::~PluginView()
{
    delete ui;
}

void PluginView::setCloseButtonVisible(bool visible)
{
    ui->closeButton->setVisible(visible);
}

void PluginView::settingsButtonClicked(bool)
{
    QModelIndex idx = ui->treeView->selectionModel()->currentIndex();
    emit itemDoubleClicked(idx);
}

void PluginView::itemDoubleClicked(QModelIndex idx)
{
    QModelIndex firstColumnIndex = idx.sibling(idx.row(),3);
    QString name = firstColumnIndex.data().toString();

    QObject *object = PluginManager::instance()->getObjectByName(name);
    if (object) {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(object);
        QDialog *dialog = 0;
        if (plugin) {
            dialog = plugin->SettingsDialog();
        }
        if (dialog) {
            dialog->exec();
            delete dialog;
            delete plugin;
        }
    }
}
