#include "salesinfo.h"
#include "qrkdelegate.h"
#include "database.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QSqlError>
#include "ui_salesinfo.h"

SalesInfo::SalesInfo(QString from, QString to, QWidget *parent) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::SalesInfo)
{
    ui->setupUi(this);

    QSqlDatabase dbc = QSqlDatabase::database("CN");

    m_salesContentModel = new QSqlQueryModel;
    ui->label->setText(tr("Umsätze %1 bis %2").arg(from).arg(to));

    QSqlQuery query(dbc);
    query.prepare(Database::getSalesPerPaymentSQLQueryString());
    query.bindValue(":fromDate", from);
    query.bindValue(":toDate", to);

    query.exec();
    m_salesContentModel->setQuery(query);

    ui->salesView->setModel(m_salesContentModel);

    m_salesContentModel->setHeaderData(0, Qt::Horizontal, tr("Bezahlart"));
    m_salesContentModel->setHeaderData(1, Qt::Horizontal, tr("Steuersatz"));
    m_salesContentModel->setHeaderData(2, Qt::Horizontal, tr("Summen"));

    ui->salesView->setItemDelegateForColumn(1, new QrkDelegate (QrkDelegate::COMBO_TAX, this));
    ui->salesView->setItemDelegateForColumn(2, new QrkDelegate (QrkDelegate::NUMBERFORMAT_DOUBLE, this));

    ui->salesView->resizeColumnsToContents();
    ui->salesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->closePushButton, SIGNAL(clicked(bool)), this, SLOT(close()));
}

SalesInfo::~SalesInfo()
{
    delete ui;
}
