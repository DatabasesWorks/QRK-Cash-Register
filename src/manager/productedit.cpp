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

#include "productedit.h"
#include "database.h"
#include "utils/utils.h"
#include "preferences/qrksettings.h"
#include "3rdparty/qbcmath/bcmath.h"
#include <ui_productedit.h>

#include <QDoubleValidator>
#include <QSqlRelationalTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDateTime>

//--------------------------------------------------------------------------------
ProductEdit::ProductEdit(QWidget *parent, int id)
    : QDialog(parent), ui(new Ui::ProductEdit), m_id(id)
{
    ui->setupUi(this);

    const QStringList colorNames = QColor::colorNames();
    int index = 0;
    ui->colorComboBox->addItem(tr("gleich wie Gruppe"));
    const QModelIndex idx = ui->colorComboBox->model()->index(index++, 0);
    ui->colorComboBox->model()->setData(idx, "", Qt::BackgroundColorRole);

    foreach (const QString &colorName, colorNames) {
        const QColor color(colorName);
        QString fg = Utils::color_best_contrast(color.name());
        const QColor fg_color(fg);
        ui->colorComboBox->addItem(colorName, color);
        const QModelIndex idx = ui->colorComboBox->model()->index(index++, 0);
        ui->colorComboBox->model()->setData(idx, color, Qt::BackgroundColorRole);
        ui->colorComboBox->model()->setData(idx, fg_color, Qt::ForegroundRole);
    }

    QJsonArray printers = Database::getPrinters();
    ui->printerComboBox->addItem(tr("gleich wie Gruppe"), 0);
    foreach (const QJsonValue & value, printers) {
        QJsonObject obj = value.toObject();
        ui->printerComboBox->addItem(obj["name"].toString(), obj["id"].toInt());
    }

    QrkSettings settings;
    int decimals = settings.value("decimalDigits", 2).toInt();
    bool useDecimals = settings.value("useDecimalQuantity", false).toBool();

    QDoubleValidator *doubleVal = new QDoubleValidator(0.0, 9999999.99, 2, this);
    doubleVal->setNotation(QDoubleValidator::StandardNotation);

    /* FIXME: QIntValitator to not restrict dots|commas (QLocale dependent)
     * QIntValidator *intVal = new QIntValidator(-9999999, 9999999, this);
     */

    QRegExp rx("-?\\d{1,6}");
    QValidator *intVal = new QRegExpValidator(rx, this);

    QRegExp rx16("\\d{1,16}");
//    ui->itemNum->setValidator(new QRegExpValidator(rx16, this));
    ui->barcode->setValidator(new QRegExpValidator(rx16, this));

    ui->net->setValidator(doubleVal);
    ui->gross->setValidator(doubleVal);
    if (useDecimals) {
        ui->stockLineEdit->setValidator(doubleVal);
        ui->minstockLineEdit->setValidator(doubleVal);
    } else {
        decimals = 0;
        ui->stockLineEdit->setValidator(intVal);
        ui->minstockLineEdit->setValidator(intVal);
    }

    QSqlDatabase dbc = Database::database();

    m_groupsModel = new QSqlRelationalTableModel(this, dbc);
    m_groupsModel->setQuery("SELECT id, name FROM groups WHERE id > 1", dbc);
    ui->groupComboBox->setModel(m_groupsModel);
    ui->groupComboBox->setModelColumn(1);  // show name

    m_taxModel = new QSqlRelationalTableModel(this, dbc);
    QString q = QString("SELECT id, tax FROM taxTypes WHERE taxlocation='%1'").arg(Database::getTaxLocation());
    m_taxModel->setQuery(q, dbc);
    ui->taxComboBox->setModel(m_taxModel);
    ui->taxComboBox->setModelColumn(1);  // show tax
    ui->taxComboBox->setCurrentIndex(0);

    m_origin = m_id;
    if ( m_id != -1 ) {
        QSqlQuery query(QString("SELECT `name`, `groupid`,`visible`,`net`,`gross`,`tax`, `color`, `itemnum`, `barcode`, `coupon`, `stock`, `minstock`, `version`, `origin`, `printerid`, `description` FROM products WHERE id=%1").arg(id), dbc);
        query.next();

        ui->name->setText(query.value("name").toString());
        ui->description->setText(query.value("description").toString());
        m_name = query.value("name").toString();
        m_itemnum = query.value("itemnum").toString();
        m_version = query.value("version").toInt();
        m_origin  = query.value("origin").toInt();
//        ui->name->setReadOnly(true);
//        ui->name->setToolTip(tr("Bestehende Artikelnamen dürfen nicht umbenannt werden. Versuchen Sie diesen Artikel zu löschen und legen danach einen neuen Artikel an."));
        ui->visibleCheckBox->setChecked(query.value("visible").toBool());
        ui->net->setText(QBCMath::bcroundL(query.value("net").toString(),2));
        ui->gross->setText(QBCMath::bcroundL(query.value("gross").toString(),2));
        ui->itemNum->setText(query.value("itemnum").toString());
        ui->barcode->setText(query.value("barcode").toString());
        ui->collectionReceiptCheckBox->setChecked(query.value("coupon").toBool());
        if (decimals == 0) {
            ui->stockLineEdit->setText(query.value("stock").toString());
            ui->minstockLineEdit->setText(query.value("minstock").toString());
        } else {
            ui->stockLineEdit->setText(QBCMath::bcroundL(query.value("stock").toString(),decimals));
            ui->minstockLineEdit->setText(QBCMath::bcroundL(query.value("minstock").toString(),decimals));
        }

        int i;
        for (i = 0; i < m_groupsModel->rowCount(); i++)
            if ( query.value("groupid").toInt() == m_groupsModel->data(m_groupsModel->index(i, 0), Qt::DisplayRole).toInt() )
                break;

        ui->groupComboBox->setCurrentIndex(i);

        for (i = 0; i < m_taxModel->rowCount(); i++)
            if ( qFuzzyCompare(query.value("tax").toDouble(), m_taxModel->data(m_taxModel->index(i, 1), Qt::DisplayRole).toDouble()) )
                break;

        ui->taxComboBox->setCurrentIndex(i);

        for (i = 0; i <= ui->colorComboBox->count(); i++) {
            QString color = ui->colorComboBox->model()->index(i, 0).data(Qt::BackgroundColorRole).toString();
            if ( query.value("color").toString() == color )
                break;
        }

        if (i > ui->colorComboBox->count())
            i = 0;

        QString colorValue = query.value("color").toString().trimmed();
        if (!colorValue.isEmpty()) {
            QPalette palette(ui->colorComboBox->palette());
            QColor color(colorValue);
            palette.setColor(QPalette::Active,QPalette::Button, color);
            palette.setColor(QPalette::Highlight, color);
            ui->colorComboBox->setPalette(palette);
        }
        ui->colorComboBox->setCurrentIndex(i);

        for (i = 0; i <= ui->printerComboBox->count(); i++) {
            if ( query.value("printerid").toInt() == i )
                break;
        }

        if (i > ui->printerComboBox->count())
          i = 0;

        ui->printerComboBox->setCurrentIndex(i);

    } else {
        ui->itemNum->setText(Database::getNextProductNumber());
    }

    connect (ui->taxComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ProductEdit::taxComboChanged);
    connect (ui->net, &QLineEdit::editingFinished, this, &ProductEdit::netChanged);
    connect (ui->gross, &QLineEdit::editingFinished, this, &ProductEdit::grossChanged);
    connect (ui->colorComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ProductEdit::colorComboChanged);
    connect (ui->okButton, &QPushButton::clicked, this, &ProductEdit::accept);
    connect (ui->cancelButton, &QPushButton::clicked, this, &ProductEdit::reject);
    connect (ui->itemNum, &QLineEdit::editingFinished, this, &ProductEdit::itemnumChanged);

    settings.beginGroup("BarcodeReader");
    m_barcodeReaderPrefix = settings.value("barcodeReaderPrefix", Qt::Key_F11).toInt();
    installEventFilter(this);
    settings.endGroup();

    bool printCollectionReceipt = settings.value("printCollectionReceipt", false).toBool();
    ui->collectionReceiptLabel->setVisible(printCollectionReceipt);
    ui->collectionReceiptCheckBox->setVisible(printCollectionReceipt);

}

ProductEdit::~ProductEdit()
{
    delete ui;
}

//--------------------------------------------------------------------------------
bool ProductEdit::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        switch (keyEvent->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            focusNextChild();
            break;

        default:
            if (keyEvent->key() == m_barcodeReaderPrefix) {
                ui->barcode->setFocus(Qt::OtherFocusReason);
                break;
            }
            return QObject::eventFilter(obj, event);
        }

        return true;
    }

    return QObject::eventFilter(obj, event);

}

void ProductEdit::colorComboChanged(int idx)
{

    QString colorValue = ui->colorComboBox->itemData(idx, Qt::BackgroundColorRole).toString(); // ->itemText(idx); //   ->model()->index(ui->colorComboBox->currentIndex(), 0).data(Qt::BackgroundColorRole).toString();
    QPalette palette(ui->colorComboBox->palette());
    QColor color(colorValue);
    palette.setColor(QPalette::Active,QPalette::Button, color);
    palette.setColor(QPalette::Highlight, color);
    ui->colorComboBox->setPalette(palette);
}

void ProductEdit::taxComboChanged(int)
{
    QBCMath tax(m_taxModel->data(m_taxModel->index(ui->taxComboBox->currentIndex(), 1)).toDouble());
    QBCMath net(ui->net->text().replace(",",".").toDouble());
    QBCMath gross(net * (1.0 + tax.toDouble() / 100.0));

    ui->gross->setText(QBCMath::bcroundL(gross.toString(), 2));
}

void ProductEdit::netChanged()
{
    QBCMath tax(m_taxModel->data(m_taxModel->index(ui->taxComboBox->currentIndex(), 1)).toDouble());
    QBCMath net(ui->net->text().replace(",",".").toDouble());
    QBCMath gross(net * (1.0 + tax.toDouble() / 100.0));

    ui->gross->setText(QBCMath::bcroundL(gross.toString(), 2));
}

void ProductEdit::grossChanged()
{
    QBCMath tax(m_taxModel->data(m_taxModel->index(ui->taxComboBox->currentIndex(), 1)).toDouble());
    QBCMath gross(ui->gross->text().replace(",",".").toDouble());
    QBCMath net(gross / (1.0 + tax.toDouble() / 100.0));

    ui->net->setText(QBCMath::bcroundL(net.toString(), 2));
}

void ProductEdit::itemnumChanged()
{
    if (ui->itemNum->text().isEmpty()) {
        ui->itemNum->setText(Database::getNextProductNumber());
        return;
    }

    if (!Utils::isNumber(ui->itemNum->text())) {
        if (Database::exists("products", ui->itemNum->text(), "itemnum"))
            ui->itemNum->setText(Database::getNextProductNumber());
        return;
    }

    if (Database::exists("products", ui->itemNum->text().toInt(), "itemnum"))
        ui->itemNum->setText(Database::getNextProductNumber());
}

void ProductEdit::accept()
{

    if (ui->name->text().isEmpty())
        return;

    int id = Database::getProductIdByName(ui->name->text());
    if (id != -1 && m_id != id) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle(tr("Artikel schon vorhanden"));
        msgBox.setText(tr("Der Artikel '%1' ist schon vorhanden. Möchten Sie diesen überschreiben?").arg(ui->name->text()));
        msgBox.setStandardButtons(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Überschreiben"));
        msgBox.setButtonText(QMessageBox::No, tr("Abbrechen"));
        msgBox.setDefaultButton(QMessageBox::No);

        if(msgBox.exec() == QMessageBox::No)
            return;

    }
    QString itemNum = ui->itemNum->text();

    if (!Utils::isNumber(itemNum)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowTitle(tr("Fehler in Artikelnummer"));
        msgBox.setText(tr("Artikelnummer ist nicht die nächste freie: %1").arg(itemNum));
        msgBox.setStandardButtons(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Übernehmen"));
        msgBox.setButtonText(QMessageBox::No, tr("Abbrechen"));
        msgBox.setDefaultButton(QMessageBox::No);

        if(msgBox.exec() == QMessageBox::No)
            return;
    }

//    m_id = id;

    bool visible = ui->visibleCheckBox->isChecked();
    if (itemNum.isEmpty()) itemNum = Database::getNextProductNumber();
    if (m_itemnum.isEmpty()) m_itemnum = Database::getNextProductNumber();

    if (m_name.compare(ui->name->text()) != 0 || m_itemnum.compare(itemNum) != 0) {
        if (m_id == -1) {
            updateData(m_id, ui->name->text(), itemNum, 0, visible);
        } else {
            if (updateData(m_id, m_name, m_itemnum, m_version, 0))
                updateData(-1, ui->name->text(), itemNum, m_version+1, visible);
        }
    } else {
        updateData(m_id, ui->name->text(), itemNum, 0, visible);
    }

    QDialog::accept();
}

//--------------------------------------------------------------------------------

bool ProductEdit::updateData(int id, QString name, QString itemnum, int version, bool visible)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    QString color = ui->colorComboBox->model()->index(ui->colorComboBox->currentIndex(), 0).data(Qt::BackgroundColorRole).toString();

    double tax = QLocale().toDouble(m_taxModel->data(m_taxModel->index(ui->taxComboBox->currentIndex(), 1)).toString());
    double net = QLocale().toDouble(ui->gross->text()) / (1.0 + tax / 100.0);
    bool collectionReceipt = ui->collectionReceiptCheckBox->isChecked();
    int printerId = ui->printerComboBox->currentData().toInt();

    if ( id == -1 )  // new entry
    {
        query.prepare(QString("INSERT INTO products (name, groupid, itemnum, barcode, visible, net, gross, tax, color, coupon, stock, minstock, version, origin, printerid, description) VALUES(:name, :group, :itemnum, :barcode, :visible, :net, :gross, :tax, :color, :coupon, :stock, :minstock, :version, :origin, :printerid, :description)"));
        if (ui->itemNum->text().isEmpty())
            ui->itemNum->setText(Database::getNextProductNumber());
    } else {
        query.prepare(QString("UPDATE products SET name=:name, itemnum=:itemnum, barcode=:barcode, groupid=:group, visible=:visible, net=:net, gross=:gross, tax=:tax, color=:color, coupon=:coupon, stock=:stock, minstock=:minstock, version=:version, origin=:origin, lastchange=:lastchange, printerid=:printerid, description=:description WHERE id=:id"));
        query.bindValue(":lastchange", QDateTime::currentDateTime());
        query.bindValue(":id", id);
    }

    query.bindValue(":name", name);
    query.bindValue(":description", ui->description->text());
    query.bindValue(":itemnum", itemnum);
    query.bindValue(":barcode", ui->barcode->text());
    query.bindValue(":group", m_groupsModel->data(m_groupsModel->index(ui->groupComboBox->currentIndex(), 0)).toInt());
    query.bindValue(":visible", visible);
    query.bindValue(":net", net);
    query.bindValue(":gross", QLocale().toDouble(ui->gross->text()));
    query.bindValue(":tax", tax);
    query.bindValue(":color", color);
    query.bindValue(":coupon", collectionReceipt);
    query.bindValue(":stock", QLocale().toDouble(ui->stockLineEdit->text()));
    query.bindValue(":minstock", QLocale().toDouble(ui->minstockLineEdit->text()));
    query.bindValue(":version", version);
    query.bindValue(":origin", m_origin);
    if (printerId > 0)
        query.bindValue(":printerid", printerId);
    else
        query.bindValue(":printerid", QVariant(QVariant::Int));

    bool ok = query.exec();
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    id = Database::getProductIdByName(name);

    if (id != -1 && m_origin == -1) {
        query.prepare(QString("UPDATE products SET origin=:origin WHERE id=:id"));
        query.bindValue(":id", id);
        query.bindValue(":origin", id);
        ok = query.exec();
    }

    return ok;
}
