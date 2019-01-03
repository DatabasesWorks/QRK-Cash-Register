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

#include "treeitem.h"
#include "treemodel.h"
#include "pluginmanager.h"
#include "pluginmanager/Interfaces/plugininterface.h"

#include <QStringList>
#include <QPluginLoader>

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << tr("Name") << tr("Version") << tr("Author") << "code";
    rootItem = new TreeItem(rootData);
    QStringList data = PluginManager::instance()->plugins();
    setupModelData(data, rootItem);
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void TreeModel::setupModelData(const QStringList &lines, TreeItem *parent)
{
    QList<TreeItem*> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].at(position) != ' ')
                break;
            position++;
        }

        QPluginLoader *loader = new QPluginLoader(lines[number]);
        QStringList columnStrings;

        QString pluginname = loader->metaData().value("MetaData").toObject().value("name").toString();
        /* The only way to get the Translated Pluginname is to load the Plugin */
        PluginInterface *plugin = qobject_cast<PluginInterface *>(PluginManager::instance()->getObjectByName(pluginname));
        if (plugin) {
            pluginname = plugin->getPluginName();
            // delete plugin;
            // plugin = 0;
        }

        columnStrings << pluginname;
        columnStrings << loader->metaData().value("MetaData").toObject().value("version").toString();
        columnStrings << loader->metaData().value("MetaData").toObject().value("author").toString();
        columnStrings << loader->metaData().value("MetaData").toObject().value("name").toString();

        QList<QVariant> columnData;
        for (int column = 0; column < columnStrings.count(); ++column)
            columnData << columnStrings[column];

        if (position > indentations.last()) {
            // The last child of the current parent is now the new parent
            // unless the current parent has no children.

            if (parents.last()->childCount() > 0) {
                parents << parents.last()->child(parents.last()->childCount()-1);
                indentations << position;
            }
        } else {
            while (position < indentations.last() && parents.count() > 0) {
                parents.pop_back();
                indentations.pop_back();
            }
        }

        // Append a new item to the current parent's list of children.
        parents.last()->appendChild(new TreeItem(columnData, parents.last()));

        ++number;
    }
}
