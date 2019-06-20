#ifndef INFOSERVER_H
#define INFOSERVER_H

#include <QObject>
#include <QUdpSocket>

#include "qrkcore_global.h"

class QRK_EXPORT InfoServer : public QObject
{
        Q_OBJECT

    public:
        InfoServer(QObject *parent = Q_NULLPTR);
        ~InfoServer();
        void run();

    signals:

    private slots:

    private:
};

#endif // INFOSERVER_H
