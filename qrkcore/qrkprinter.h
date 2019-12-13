#ifndef QRKPRINTER_H
#define QRKPRINTER_H

#include "qrkcore_global.h"

#include <QObject>
#include <QPrinter>

class QRK_EXPORT QRKPrinter : public QObject
{
    Q_OBJECT

public:
    explicit QRKPrinter(QString printername, QObject *parent = Q_NULLPTR);
    explicit QRKPrinter(int printerid, QObject *parent = Q_NULLPTR);
    ~QRKPrinter();
    QList<QPrinter *> *getPrinterList();

private:
    void setDefaultPDFPrinter();
    void setDefinition(const QJsonObject &object, QPrinter *&printer);
    void initPrinters();
    QString m_printername;
    QList<QPrinter *> *m_printers;
};

#endif // QRKPRINTER_H
