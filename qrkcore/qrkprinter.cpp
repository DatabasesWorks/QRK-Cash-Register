#include "qrkprinter.h"
#include "database.h"

#include <QVariant>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QPrinterInfo>

#include <QDebug>

QRKPrinter::QRKPrinter(QString printername, QObject *parent) : QObject(parent), m_printername(printername)
{
    initPrinters();
}

QRKPrinter::QRKPrinter(int printerid, QObject *parent) : QObject(parent), m_printername(Database::getPrinterName(printerid))
{
    initPrinters();
}

QRKPrinter::~QRKPrinter()
{
    while (!m_printers->isEmpty()) {
        delete m_printers->takeFirst();
    }
    delete m_printers;
    m_printers = Q_NULLPTR;
}

void QRKPrinter::initPrinters()
{
    m_printers = new QList<QPrinter *>();
    if (m_printername.isEmpty()) m_printername = "QrkPDF";
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT printer FROM printers WHERE name=:name");
    query.bindValue(":name", m_printername);
    query.exec();
    while (query.next()) {
        QByteArray data = QByteArray::fromBase64(query.value("printer").toByteArray());
        QJsonArray jArray = QJsonDocument().fromBinaryData(data).array();
        QSqlQuery definition(dbc);
        definition.prepare("SELECT definition FROM printerdefs WHERE id=:id");
        foreach (const QJsonValue &value, jArray) {
            QJsonObject obj = value.toObject();
            QPrinter *printer = new QPrinter;
            printer->setPrinterName(obj["name"].toString());
            m_printername = obj["name"].toString();
            definition.bindValue(":id", obj["definitionid"].toInt());
            definition.exec();
            if (definition.next()) {
                QByteArray data = QByteArray::fromBase64(definition.value("definition").toByteArray());
                setDefinition(QJsonDocument().fromBinaryData(data).object(), printer);
                qDebug() << "Function Name: " << Q_FUNC_INFO << "PageSize: " << printer->pageSizeMM();
                qDebug() << "Function Name: " << Q_FUNC_INFO << "PaperSize: " << printer->paperSize(QPrinter::Millimeter);

                m_printers->append(printer);
            }
        }
    }
    if (m_printers->isEmpty())
        setDefaultPDFPrinter();
}

void QRKPrinter::setDefaultPDFPrinter()
{
    QPrinter *printer = new QPrinter;
    printer->setFullPage(true);

    printer->setOutputFormat(QPrinter::OutputFormat::PdfFormat);
    printer->setPageSize(QPrinter::A4);
    printer->setFullPage(false);
    m_printers->append(printer);
}

void QRKPrinter::setDefinition(const QJsonObject &object, QPrinter *&printer)
{
    printer->setFullPage(true);

    qDebug() << "Function Name: " << Q_FUNC_INFO <<  "printername: " << printer->printerName();

    QPrinterInfo pInfo = QPrinterInfo::printerInfo(printer->printerName());
    QList<QPageSize> pSizes = pInfo.supportedPageSizes();

    qDebug() << "Function Name: " << Q_FUNC_INFO <<  "maximum: " << pInfo.maximumPhysicalPageSize();

    bool hasCustomFormat = false;
    QListIterator<QPageSize> iter(pSizes);
    while(iter.hasNext()){
        QPageSize pageSize = iter.next();
        if (pageSize.id() == static_cast<int>(QPrinter::Custom)) {
            hasCustomFormat = true;
            break;
        }
    }

    if(object["pdf"].toBool())
        printer->setOutputFormat(QPrinter::OutputFormat::PdfFormat);

    if (object["type"].toString() == "custom" && hasCustomFormat) {
        QSizeF sizeF(object["paperWidth"].toInt(),object["paperHeight"].toInt());
        printer->setPaperSize(sizeF, QPrinter::Millimeter);
    } else if (object["type"].toString() == "A4" && pSizes.contains(QPageSize(QPageSize::A4))) {
        printer->setPaperSize(printer->A4);
    } else if (object["type"].toString() == "A5" && pSizes.contains(QPageSize(QPageSize::A5))) {
        printer->setPaperSize(printer->A5);
    } else {
        printer->setPageSize(pInfo.defaultPageSize());
        qDebug() << "Function Name: " << Q_FUNC_INFO <<  "default PageSize: " << pInfo.defaultPageSize();
        qDebug() << "Function Name: " << Q_FUNC_INFO <<  "PaperSize: " << printer->paperSize(QPrinter::Millimeter);
    }

    if (m_printername == "QrkPDF")
        printer->setOutputFormat(QPrinter::OutputFormat::PdfFormat);

    // printer.setResolution(720);

    const QMarginsF marginsF(object["marginLeft"].toDouble(),
                             object["marginTop"].toDouble(),
                             object["marginRight"].toDouble(),
                             object["marginBottom"].toDouble());

    printer->setPageMargins(marginsF,QPageLayout::Millimeter);

    printer->setFullPage(false);

}

QList<QPrinter *> *QRKPrinter::getPrinterList()
{
    return m_printers;
}
