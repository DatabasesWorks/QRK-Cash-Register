#ifndef EXPORT_H
#define EXPORT_H

#include "qrkcore_global.h"
#include "singleton/spreadsignal.h"

#include <QObject>

class QRK_EXPORT Export : public QObject
{
        Q_OBJECT

    public:
        explicit Export(QObject *parent = nullptr);
        ~Export();

        bool depExport(QString filename);
        bool createBackup();
        bool createBackup(int &counter);
        static int getLastMonthReceiptId();

    protected:
        bool depExport(QString outputFile, QString from, QString to);
        QJsonDocument depExport(int from, int to);
        QJsonDocument mapExport();
        QJsonArray getReceipts(int from, int to);
};

#endif // EXPORT_H
