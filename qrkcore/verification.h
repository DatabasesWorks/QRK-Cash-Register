#ifndef VERIFICATION_H
#define VERIFICATION_H

#include "qrkcore_global.h"
#include <QObject>

class QRK_EXPORT Verification: public QObject
{
    Q_OBJECT

public:
    Verification(QObject *parent = Q_NULLPTR);
    QString readKeyFromFile(const QString &filename);
    QJsonObject readJsonFromString(const QString &data);
    QJsonObject readJsonFromFile(const QString &filename);
    QString getSignedText(const QJsonObject &obj);
};

#endif // VERIFICATION_H
