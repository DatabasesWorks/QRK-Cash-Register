#include "database.h"
#include "qrkgastroquickproduct.h"
#include "ui_qrkgastroquickproduct.h"

#include <QSqlQuery>
#include <QCompleter>
#include <QJsonObject>
#include <QDebug>

QrkGastroQuickProduct::QrkGastroQuickProduct(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QrkGastroQuickProduct)
{
    ui->setupUi(this);
    setTaxes();
    setCompleter();
    QDoubleValidator *doubleVal = new QDoubleValidator(0.0, 9999999.99, 2, this);
    doubleVal->setNotation(QDoubleValidator::StandardNotation);
    ui->grossLineEdit->setValidator(doubleVal);

    connect(ui->cancelPushButton, &QPushButton::clicked, this, &QrkGastroQuickProduct::close);
    connect(ui->okPushButton, &QPushButton::clicked, this, &QrkGastroQuickProduct::accept);
}

QrkGastroQuickProduct::~QrkGastroQuickProduct()
{
    delete ui;
}

void QrkGastroQuickProduct::setTaxes()
{
    QString taxlocation = Database::getTaxLocation();
    QSqlDatabase dbc= Database::database();
    QSqlQuery query(dbc);
    query.prepare(QString("SELECT tax FROM taxTypes WHERE taxlocation=:taxlocation ORDER BY id"));
    query.bindValue(":taxlocation", taxlocation);
    if(!query.exec()){
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Can't get taxType list! Taxlocation = " << taxlocation;
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query:" << Database::getLastExecutedQuery(query);
    }
    while(query.next()){
        ui->taxComboBox->addItem(query.value(0).toString());
    }
    ui->taxComboBox->setCurrentIndex(ui->taxComboBox->findText(Database::getDefaultTax()));
}

void QrkGastroQuickProduct::setCompleter()
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("select DISTINCT p2.name from (select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where groupid=2 AND visible = 0");
    query.exec();

    QStringList list;

    while(query.next()){
        QString value = query.value("name").toString();
        list << value;
    }

    QCompleter *editorCompleter = new QCompleter( list ) ;
    editorCompleter->setCaseSensitivity( Qt::CaseInsensitive ) ;
    editorCompleter->setFilterMode( Qt::MatchContains );
    ui->productLineEdit->setCompleter( editorCompleter );
}

void QrkGastroQuickProduct::accept()
{
    if (ui->productLineEdit->text().trimmed().isEmpty()) {
        ui->productLineEdit->setFocus();
        return;
    }

    if (ui->grossLineEdit->text().trimmed().isEmpty()) {
        ui->grossLineEdit->setFocus();
        return;
    }

    QJsonObject itemdata;
    itemdata["name"] = ui->productLineEdit->text().trimmed();
    itemdata["itemnum"] = "D"; // D = Dummy Product No itemnumber Update
    itemdata["tax"] = ui->taxComboBox->currentText().toDouble();
    itemdata["net"] = 0.0;
    itemdata["gross"] = QLocale().toDouble(ui->grossLineEdit->text().trimmed());
    itemdata["visible"] = 0;
    itemdata["group"] = 2;

    if (Database::addProduct(itemdata)) {
        m_productId = Database::getProductIdByName(itemdata["name"].toString());
        Database::updateProductPrice(itemdata["gross"].toDouble(), itemdata["name"].toString());
        Database::updateProductTax(itemdata["tax"].toDouble(), itemdata["name"].toString());
    }

    emit QDialog::accept();
}

int QrkGastroQuickProduct::getProductId()
{
    return  m_productId;
}
