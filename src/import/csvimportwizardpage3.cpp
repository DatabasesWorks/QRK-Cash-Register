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

#include "csvimportwizardpage3.h"
#include "ui_csvimportwizardpage3.h"
#include "database.h"
#include "backup.h"

#include <QTableView>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QDateTime>
#include <QJsonObject>
#include <QDebug>

CsvImportWizardPage3::CsvImportWizardPage3(QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::CsvImportWizardPage3)
{
    ui->setupUi(this);
    connect(ui->startImportButton, &QPushButton::clicked, this, &CsvImportWizardPage3::save);
}

CsvImportWizardPage3::~CsvImportWizardPage3()
{
    delete ui;
}

bool CsvImportWizardPage3::isComplete() const
{
    return m_finished;
}

void CsvImportWizardPage3::initializePage()
{

    m_finished = false;
    ui->startImportButton->setEnabled(true);
    m_autoGroup = field("autogroup").toBool();
    m_guessGroup = field("guessgroup").toBool();
    m_ignoreExistingProduct = field("ignoreexisting").toBool();
    m_updateExistingProduct = field("updateexisting").toBool();
    m_importType = field("importtype").toString();

    m_visibleGroup = field("visiblegroup").toBool();
    m_visibleProduct = field("visibleproduct").toBool();

    m_autoitemnum = field("autoitemnum").toBool();
    m_autominitemnum = field("autominitemnum").toInt();

}

void CsvImportWizardPage3::setModel(QStandardItemModel *model)
{
    m_model =  model;

}

void CsvImportWizardPage3::setMap(QMap<QString, QVariant> *map)
{
    m_map =  map;
}

void CsvImportWizardPage3::setMap(QMap<QString, QJsonObject> *map)
{
    m_errormap =  map;
}

void CsvImportWizardPage3::importFinished()
{
    m_finished = true;

    if (m_errormap->isEmpty()) {
        emit setFinalPage(true);
        int id = nextId();
        if (id != -1)
            emit removePage(id);
    }

    emit completeChanged();
    emit info(tr("Der Datenimport wurde abgeschlossen."));

}

void CsvImportWizardPage3::info(QString info)
{
    info = QString("%1 %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(info);
    ui->listWidget->addItem(info);
    ui->listWidget->scrollToBottom();
}

void CsvImportWizardPage3::save(bool)
{
    ui->startImportButton->setEnabled(false);
    emit info(tr("Backup der Daten wurde gestartet."));
    Backup::create();
    emit info(tr("Backup der Daten fertig."));
//    emit completeChanged();
    emit info(tr("Der DatenImport wurde gestarted."));
    ImportData *import = new ImportData(m_model, m_map, m_errormap, m_ignoreExistingProduct, m_guessGroup, m_autoGroup, m_visibleGroup, m_visibleProduct, m_updateExistingProduct, m_autoitemnum, m_autominitemnum);
    m_thread = new QThread;
    import->moveToThread(m_thread);

    connect(m_thread, &QThread::started, import, &ImportData::run);

    connect(import, &ImportData::percentChanged, ui->progressBar, &QProgressBar::setValue);
    connect(import, &ImportData::info, this, &CsvImportWizardPage3::info);
    connect(import, &ImportData::finished, this, &CsvImportWizardPage3::importFinished);
    connect(import, &ImportData::finished, import, &ImportData::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);

    m_thread->start();

}

void CsvImportWizardPage3::cancel()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->requestInterruption();
    }
}

//---- ImportData ------------------------------------------------------------------------------------------------------

ImportData::ImportData(QStandardItemModel *model, QMap<QString, QVariant> *map, QMap<QString, QJsonObject> *errormap, bool ignoreExistingProduct, bool guessGroup, bool autoGroup,  bool visibleGroup, bool visibleProduct, bool updateExistingProduct, bool autoitemnum, int autominitemnum)
{
    m_model = model;
    m_map = map;
    m_errormap = errormap;
    m_autoGroup = autoGroup;
    m_guessGroup = guessGroup;
    m_ignoreExistingProduct = ignoreExistingProduct;
    m_updateExistingProduct = updateExistingProduct;
    m_visibleGroup = visibleGroup;
    m_visibleProduct = visibleProduct;
    m_autoitemnum = autoitemnum;
    m_autominitemnum = autominitemnum;

}

ImportData::~ImportData()
{
}

void ImportData::run()
{

    int count = m_model->rowCount();
    // count = 100; /* for testing */

    for (int row = 0; row < count; row++) {
        if(QThread::currentThread()->isInterruptionRequested())
            return;

        m_origin = -1;
        m_version = 0;

        int percent = int((float(row) / float(count)) * 100);
        emit percentChanged(percent);

        QString itemnum = getItemValue(row, m_map->value(tr("Artikelnummer")).toInt());
        QString barcode = getItemValue(row, m_map->value(tr("Barcode")).toInt());
        QString name = getItemValue(row, m_map->value(tr("Artikelname")).toInt());
        QString color = getItemValue(row, m_map->value(tr("Farbe")).toInt());
        QString coupon = getItemValue(row, m_map->value(tr("Extrabon")).toInt());
        double stock = getItemValue(row, m_map->value(tr("Lagerbestand")).toInt(),true).toDouble();
        double minstock = getItemValue(row, m_map->value(tr("Mindestbestand")).toInt(),true).toDouble();
        double sold = getItemValue(row, m_map->value(tr("Verkauft")).toInt(),true).toDouble();
        QString visible = getItemValue(row, m_map->value(tr("Sichtbar")).toInt());

        int type = 0;
        if (m_ignoreExistingProduct && exists(itemnum, barcode, name, type)) {
            emit info(tr("%1 ist schon vorhanden. Der Import wurde ignoriert").arg(name));
            continue;
        }

        double net = getItemValue(row, m_map->value(tr("Netto Preis")).toInt(),true).toDouble();
        double gross = getItemValue(row, m_map->value(tr("Brutto Preis")).toInt(),true).toDouble();
        double tax;
        if (m_map->value(tr("Steuersatz")).toInt() == 0)
            tax = Database::getDefaultTax().toDouble();
        else
            tax = getItemValue(row, m_map->value(tr("Steuersatz")).toInt(),true).toDouble();

        if (tax < 1) tax = tax * 100.0;
        if (gross < net) gross = net * (1.0 + tax / 100.0);
        if (gross != 0.00 && net == 0.00) net = gross / (1.0 + tax / 100.0);

        int errortype = 0;
        int id = exists(itemnum, barcode, name, errortype);

        QJsonObject j;
        j["itemnum"] = itemnum;
        j["barcode"] = barcode;
        j["name"] = name;
        j["color"] = color;
        j["coupon"] = coupon;
        j["stock"] = stock;
        j["minstock"] = minstock;
        j["net"] = net;
        j["gross"] = gross;
        j["sold"] = sold;
        j["tax"] = tax;
        j["row"] = row;

        if (!m_updateExistingProduct && id == 0) {
            int existId;
            if (errortype == 1) {
                existId = Database::getProductIdByNumber(itemnum);
                m_errormap->insert(QString("itemnum, %1").arg(existId), j);
            } else if (errortype == 2){
                existId = Database::getProductIdByBarcode(barcode);
                m_errormap->insert(QString("barcode, %1").arg(existId), j);
            }
            continue;
        }

        m_realVisible = m_visibleProduct;

        if (m_updateExistingProduct && id > 0) {
            QJsonObject currentProduct = Database::getProductById(id, 0);
            if (itemnum.isEmpty()) {
                itemnum = currentProduct.value("itemnum").toString();
                j["itemnum"] = itemnum;
            }

            if (itemnum.compare(currentProduct.value("itemnum").toString()) != 0 || name.compare(currentProduct.value("name").toString()) != 0 ) {
                m_realVisible = 0;
                updateData(id, currentProduct);
                m_realVisible = m_visibleProduct;
                if (visible.isEmpty()) {
                    m_realVisible = visible.toInt();
                }
                m_origin = currentProduct.value("origin").toInt();
                m_version = currentProduct.value("version").toInt() +1;
                updateData(-1, j);
            } else {
                m_version = currentProduct.value("version").toInt();
                updateData(-1, j);
            }
            emit info(tr("Update %1 (version = %2)").arg(name).arg(m_version));
        } else {
            updateData(id, j);
        }
    }

    emit percentChanged(100);
    emit finished();

}

QString ImportData::getItemValue(int row, int col, bool replace)
{
    col--;
    if (col >= 0) {
        QStandardItem *item;
        item = m_model->item(row, col);
        if (!item)
            return "";

        if (replace)
            return item->text().remove(QRegExp("[^0-9,.]")).replace(",",".");

        QString text = item->text();
        if (text.isNull())
            return "";
        return item->text();
    }

    return "";

}

QString ImportData::getGroupById(int id)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT name FROM groups WHERE id=:id");
    query.bindValue(":id", id);

    query.exec();
    if (query.next())
        return query.value(0).toString();

    return "";
}

int ImportData::getGroupByName(QString name)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT id FROM groups WHERE name=:name");
    query.bindValue(":name", name);

    query.exec();
    if (query.next())
        return query.value(0).toInt();

    return -1;

}

QString ImportData::getGuessGroup(QString name)
{
    QStringList splitedNames = name.split(" ");
    QString group = splitedNames.at(0);
    if (group.endsWith('.') && splitedNames.size() > 1)
        group.append(" " + splitedNames.at(1));

    return group;
}

int ImportData::createGroup(QString name)
{

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT id FROM groups WHERE name=:name"));
    query.bindValue(":name", name);

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    if(query.next())
        return query.value(0).toInt();


    query.prepare(QString("INSERT INTO groups (name, visible) VALUES(:name, :visible)"));
    query.bindValue(":name", name);
    query.bindValue(":visible", (m_visibleGroup)? 1: 0);

    ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    emit info(tr("Gruppe %1 wurde erstellt.").arg(name));

    return getGroupByName(name);

}

int ImportData::exists(QString itemnum, QString barcode, QString name, int &type)
{

    if (name.isEmpty())
        return 0;

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    if (!itemnum.isEmpty()) {
        // bool ok = query.prepare("SELECT id, name FROM products WHERE itemnum=:itemnum");
        bool ok = query.prepare("select p2.id, p2.name, p2.version, p2.origin from (select max(version) as version, origin from products group by origin) p1 inner join (select id, name, itemnum, version, origin from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where itemnum=:itemnum");

        query.bindValue(":itemnum", itemnum);

        if (!ok) {
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
        }

        query.exec();
        if (query.next()) {
            m_origin = query.value("origin").toInt();
            m_version = query.value("version").toInt();
            if (!m_updateExistingProduct && query.value("name").toString() != name) {
                emit info(tr("Artikelnummer %1 (%2) ist bereits für Artikel %3 vergeben. Kein Import möglich.").arg(itemnum).arg(name).arg(query.value("name").toString()));
                type = 1;
                return 0;
            }
            return query.value("id").toInt();
        }
    }
    if (!barcode.isEmpty()) {

        // bool ok = query.prepare("SELECT id, name FROM products WHERE barcode=:barcode");
        bool ok = query.prepare("select p2.id, p2.name, p2.version, p2.origin from (select max(version) as version, origin from products group by origin) p1 inner join (select id, name, barcode, version, origin from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where barcode=:barcode");

        query.bindValue(":barcode", barcode);

        if (!ok) {
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
        }

        query.exec();
        if (query.next()) {
            m_origin = query.value("origin").toInt();
            m_version = query.value("version").toInt();
            if (!m_updateExistingProduct && query.value("name").toString() != name) {
                emit info(tr("Barcode %1 (%2) ist bereits für Artikel %3 vergeben. Kein Import möglich.").arg(barcode).arg(name).arg(query.value(1).toString()));
                type = 2;
                return 0;
            }

            return query.value("id").toInt();
        }
    }

//    bool ok = query.prepare("SELECT id FROM products WHERE name=:name");
    bool ok = query.prepare("select p2.id, p2.name, p2.version, p2.origin from (select max(version) as version, origin from products group by origin) p1 inner join (select id, name, version, origin from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where name=:name");

    query.bindValue(":name", name);
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.exec();
    if (query.next()) {
        m_origin = query.value("origin").toInt();
        m_version = query.value("version").toInt();
        return query.value("id").toInt();
    }

    return -1;
}

bool ImportData::updateData(int id, QJsonObject data)
{
    qApp->processEvents();
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    dbc.transaction();

    bool ok = false;

    if (m_updateExistingProduct && id > 0) {
        ok= query.prepare("UPDATE products SET name=:name, itemnum=:itemnum, barcode=:barcode, sold=:sold, tax=:tax, net=:net, gross=:gross, visible=:visible, color=:color, coupon=:coupon, stock=:stock, minstock=:minstock, version=:version, origin=:origin, groupid=:group WHERE id=:id");
        query.bindValue(":id", id);
        emit info(tr("Update %1").arg(data.value("name").toString()));
    } else {
        ok= query.prepare("INSERT INTO products (name, groupid, itemnum, barcode, sold, visible, net, gross, tax, color, coupon, stock, minstock, version, origin) VALUES (:name, :group, :itemnum, :barcode, :sold, :visible, :net, :gross, :tax, :color, :coupon, :stock, :minstock, :version, :origin)");
    }

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    QString itemnum = data.value("itemnum").toString();

    if (m_autoitemnum && itemnum.isEmpty()) {
        int i = Database::getNextProductNumber().toInt();
        itemnum = QString::number((m_autominitemnum < i)?i:m_autominitemnum);
    }

    query.bindValue(":itemnum", itemnum);
    query.bindValue(":barcode", data.value("barcode").toString());
    query.bindValue(":name", data.value("name").toString());
    query.bindValue(":sold", data.value("sold").toInt());
    query.bindValue(":net", data.value("net").toDouble());
    query.bindValue(":gross", data.value("gross").toDouble());
    query.bindValue(":tax", data.value("tax").toDouble());
    query.bindValue(":visible", m_realVisible);
    query.bindValue(":color", data.value("color").toString());
    query.bindValue(":coupon", (data.value("coupon").toString().toInt() == 0)? false: true);
    query.bindValue(":stock", data.value("stock").toDouble());
    query.bindValue(":minstock", data.value("minstock").toDouble());
    query.bindValue(":version", m_version);
    query.bindValue(":origin", m_origin);

    int row = data.value("row").toInt();
    QString group = getItemValue(row, m_map->value(tr("Gruppe")).toInt());
    if (group.toInt() > 0) {
        if (getGroupByName(getGroupById(group.toInt())) == group.toInt()) {
            query.bindValue(":group", group.toInt());
        } else {
            if(m_guessGroup) {
                int id = createGroup(getGuessGroup(data.value("name").toString()));
                query.bindValue(":group", id);
            } else {
                query.bindValue(":group", 2);
            }
        }
    } else {
        if (group.isEmpty()) {
            if(m_guessGroup) {
                int id = createGroup(getGuessGroup(data.value("name").toString()));
                query.bindValue(":group", id);
            } else {
                query.bindValue(":group", 2);
            }
        } else {
            int id = getGroupByName(group);
            if (id > 0) {
                query.bindValue(":group", id);
            } else {
                if(m_autoGroup) {
                    int id = createGroup(group);
                    query.bindValue(":group", id);
                } else {
                    query.bindValue(":group", 2);
                }
            }
        }
    }

    ok = query.exec();
    id = Database::getProductIdByName(data.value("name").toString());

    if (id != -1 && m_origin == -1) {
        query.prepare(QString("UPDATE products SET origin=:origin WHERE id=:id"));
        query.bindValue(":id", id);
        query.bindValue(":origin", id);
        ok = query.exec();
    }

    if (!dbc.commit())
        dbc.rollback();

    qDebug() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }
    return ok;
}
