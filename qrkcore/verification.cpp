#include "verification.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>

Verification::Verification(QObject *parent): QObject(parent)
{

}

QJsonObject Verification::readJsonFromString(const QString &data)
{

    QStringList list = data.split("\n");
    QListIterator<QString> iter(list);
    QString json = "";
    while(iter.hasNext()){
        QString line = iter.next();
        if (line.startsWith("----"))
            continue;
        json.append(line);
     }
     return QJsonDocument::fromJson(json.toUtf8()).object();
}

QJsonObject Verification::readJsonFromFile(const QString &filename)
{
    QFile inputFile(filename);
    if (inputFile.open(QIODevice::ReadOnly)) {
        QString json;
       QTextStream in(&inputFile);
       while (!in.atEnd()) {
          QString line = in.readLine();
          if (line.startsWith("----"))
              continue;
          json.append(line);
       }
       inputFile.close();
       return QJsonDocument::fromJson(json.toUtf8()).object();
    }

    return QJsonObject();
}

QString Verification::getSignedText(const QJsonObject &obj)
{
    return  obj["CashRegisterId"].toString() + ";"
            + obj["OrderMail"].toString() + ";"
            + obj["Date"].toString() + ";"
            + obj["Customer"].toString() + ";"
            + obj["Product"].toString() + ";"
            + obj["ValidTill"].toString() + ";"
            + obj["SerialNumber"].toString() + ";";
}

QString Verification::readKeyFromFile(const QString &filename)
{
    QFile file(filename);
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream in(&file);
    return in.readAll();
}
