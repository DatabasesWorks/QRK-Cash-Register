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

#include "csvimportwizardpage4.h"
#include "ui_csvimportwizardpage4.h"
#include "database.h"
#include "backup.h"

#include <QTableView>

// #include <QSqlQuery>
// #include <QSqlError>
// #include <QThread>
#include <QDateTime>
#include <QJsonObject>
#include <QDebug>

CsvImportWizardPage4::CsvImportWizardPage4(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::CsvImportWizardPage4)
{
    ui->setupUi(this);
    connect(ui->cancelImportButton, &QPushButton::clicked, this, &CsvImportWizardPage4::cancelButton);
    connect(ui->newImportButton, &QPushButton::clicked, this, &CsvImportWizardPage4::newButton);
    connect(ui->changeImportButton, &QPushButton::clicked, this, &CsvImportWizardPage4::changeButton);
}

CsvImportWizardPage4::~CsvImportWizardPage4()
{
    delete ui;
}

bool CsvImportWizardPage4::isComplete() const
{
    return m_errormap->isEmpty();
}

void CsvImportWizardPage4::initializePage()
{

    m_autoGroup = field("autogroup").toBool();
    m_guessGroup = field("guessgroup").toBool();
    m_ignoreExistingProduct = field("ignoreexisting").toBool();
    m_updateExistingProduct = field("updateexisting").toBool();
    m_importType = field("importtype").toString();

    m_visibleGroup = field("visiblegroup").toBool();
    m_visibleProduct = field("visibleproduct").toBool();

    ui->changeImportButton->setHidden(true);

    setWidgetData();

}

void CsvImportWizardPage4::setWidgetData()
{
    ui->treeWidget->clear();
    QMapIterator<QString, QJsonObject> iter(*m_errormap);
    while(iter.hasNext())
    {
        iter.next();
        QStringList list = iter.key().split(",");
        QJsonObject j = iter.value();
        QJsonObject original = Database::getProductByName(Database::getProductNameById(list.at(1).trimmed().toInt()), 0);
        QTreeWidgetItem *top_item = new QTreeWidgetItem;
        top_item->setFlags(top_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        top_item->setCheckState(0, Qt::Checked);
        ui->treeWidget->addTopLevelItem(top_item);
        QTreeWidgetItem* child_item = new QTreeWidgetItem;
        QString info = QString(list.at(0)).replace("itemnum", tr("Artikelnummer").replace("barcode", tr("Barcode")));

        if (info == tr("Barcode"))
            info = info + QString(" (%1)").arg(original["barcode"].toString());
        else
            info = info + QString(" (%1)").arg(original["itemnum"].toString());

        top_item->setText(0, tr("Artikel %1 konnte nicht importiert werden. \"%2\" ist schon vergeben.").arg(j["name"].toString()).arg(info));
        top_item->setData(0, Qt::UserRole, iter.key());
        child_item->setText(0, tr("Artikelnummer: \"%1\", Barcode: \"%2\" vergeben an Artikelname \"%3\" ").arg(original["itemnum"].toString()).arg(original["barcode"].toString()).arg(original["name"].toString()));
        top_item->addChild(child_item);

        if (Database::exists(j["name"].toString())) {
            QTreeWidgetItem* child2_item = new QTreeWidgetItem;
            QJsonObject object = Database::getProductByName(j["name"].toString(), 0);
            QString itemnum = object["itemnum"].toString();
            child2_item->setText(0, tr("Artikelname \"%1\" ist schon vorhanden und hat die \"Artikelnummer %2\"").arg(j["name"].toString()).arg((itemnum.isEmpty())?tr("leer"):itemnum));
            top_item->addChild(child2_item);
            if (itemnum == original["itemnum"].toString()) {
                QTreeWidgetItem* child3_item = new QTreeWidgetItem;
                child3_item->setTextColor(0, Qt::red);
                child3_item->setText(0, tr("Mehrfach Verwendung von Artikelnummern. Bitte bereinigen Sie Ihre Daten im Artikemmanager oder in der Importdatei"));
                top_item->addChild(child3_item);
            }
        }
    }
}

void CsvImportWizardPage4::setMap(QMap<QString, QJsonObject> *map)
{
    m_errormap =  map;
}

void CsvImportWizardPage4::importFinished()
{
    if (m_errormap->isEmpty()) {
        ui->cancelImportButton->setEnabled(false);
        ui->changeImportButton->setEnabled(false);
        ui->newImportButton->setEnabled(false);

        emit info(tr("Der Datenimport wurde abgeschlossen. Keine weiteren Artikel vorhanden."));
        emit completeChanged();
    }
}

void CsvImportWizardPage4::info(QString info)
{
    info = QString("%1 %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(info);
    ui->infoLabel->setText(info);
//    ui->treeWidget->addItem(info);
}

void CsvImportWizardPage4::newButton(bool)
{
    QList<QTreeWidgetItem*> itemList;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
        itemList << ui->treeWidget->topLevelItem(i);

    QListIterator<QTreeWidgetItem*> Iter( itemList );
    while( Iter.hasNext() ) {
        QTreeWidgetItem *item = Iter.next();
        if (item->checkState(0) == Qt::Checked) {
            QVariant data = item->data(0, Qt::UserRole);
            QJsonObject j = m_errormap->value(data.toString());
            if (j.isEmpty())
                continue;

            j["itemnum"] = Database::getNextProductNumber();
            if (Database::addProduct(j)) {
                if (m_errormap->contains(data.toString())) {
                    m_errormap->remove(data.toString());
                }
            }
        }
    }

    setWidgetData();
    emit importFinished();

}

void CsvImportWizardPage4::changeButton(bool)
{
}

void CsvImportWizardPage4::cancelButton(bool)
{
    QList<QTreeWidgetItem*> itemList;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
        itemList << ui->treeWidget->topLevelItem(i);

    QListIterator<QTreeWidgetItem*> Iter( itemList );
    while( Iter.hasNext() ) {
        QTreeWidgetItem *item = Iter.next();
        if (item->checkState(0) == Qt::Checked) {
            QVariant data = item->data(0, Qt::UserRole);
            if (m_errormap->contains(data.toString())) {
                m_errormap->remove(data.toString());
            }
        }
    }

    setWidgetData();
    emit importFinished();
}
