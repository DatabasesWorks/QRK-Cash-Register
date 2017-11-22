#include "export.h"

#include "RK/rk_signaturemodule.h"
#include "utils/utils.h"
#include "preferences/qrksettings.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QDebug>

Export::Export(QObject *parent) : QObject(parent)
{

}

Export::~Export()
{
    SpreadSignal::setProgressBarValue(-1);
}

QJsonDocument Export::depExport(int from, int to)
{
    QJsonObject root;
    QJsonArray group;
    QJsonObject object;

    object["Belege-kompakt"] = getReceipts(from, to);
    object["Signaturzertifikat"] = "";

    group.append(object);

    root["Belege-Gruppe"] = group;

    QJsonDocument doc(root);

    return doc;
}

QJsonDocument Export::mapExport()
{
    QJsonObject root;
    QJsonObject map;

    map = RKSignatureModule::getCertificateMap();

    root["base64AESKey"] = RKSignatureModule::getPrivateTurnoverKeyBase64();
    root["certificateOrPublicKeyMap"] = map;

    QJsonDocument doc(root);

    return doc;
}

QJsonArray Export::getReceipts(int from, int to)
{
    QJsonArray receipts;
    QSqlDatabase dbc = QSqlDatabase::database("CN");
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT data FROM dep WHERE receiptNum BETWEEN :from AND :to ORDER by id"));
    query.bindValue(":from", from);
    query.bindValue(":to", to);

    query.exec();

    int i = 0;
    int count = query.record().count();

    while (query.next()) {
        i++;
        SpreadSignal::setProgressBarValue(((float)i / (float)count) * 100);
        receipts.append(query.value(0).toString());
    }

    return receipts;
}

bool Export::depExport(QString filename)
{
    QFile outputFile(filename);

    /* Try and open a file for output */
    outputFile.open(QIODevice::WriteOnly | QIODevice::Text);

    /* Check it opened OK */
    if(!outputFile.isOpen()){
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error, unable to open" << filename << "for output";
        return false;
    }

    int beginID = 1;
    int endID = getLastMonthReceiptId();

    QJsonDocument dep = depExport(beginID, endID);
    QTextStream outStreamDEP(&outputFile);
    outStreamDEP << dep.toJson();
    /* Close the file */
    outputFile.close();

    if (endID == -1)
        return false;

    return true;
}

bool Export::depExport(QString outputDir, QString from, QString to)
{
    QString filenameDEPExport = QDir::toNativeSeparators(outputDir + "/dep-export.json");
    QString filenameMAPExport = QDir::toNativeSeparators(outputDir + "/cryptographicMaterialContainer.json");

    QFile outputFileDEP(filenameDEPExport);
    QFile outputFileMAP(filenameMAPExport);

    /* Try and open a file for output */
    outputFileDEP.open(QIODevice::WriteOnly | QIODevice::Text);
    outputFileMAP.open(QIODevice::WriteOnly | QIODevice::Text);

    /* Check it opened OK */
    if(!outputFileDEP.isOpen()){
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error, unable to open" << outputFileDEP.fileName() << "for output";
        return false;
    }

    /* Check it opened OK */
    if(!outputFileMAP.isOpen()){
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error, unable to open" << outputFileDEP.fileName() << "for output";
        return false;
    }

    /* Point a QTextStream object at the file */
    QTextStream outStreamDEP(&outputFileDEP);
    QTextStream outStreamMAP(&outputFileMAP);

    QSqlDatabase dbc = QSqlDatabase::database("CN");
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT MIN(receiptNum) as begin, MAX(receiptNum) as end FROM receipts WHERE timestamp BETWEEN :fromDate AND :toDate"));
    query.bindValue(":fromDate", from);
    query.bindValue(":toDate", to);
    query.exec();

    if (query.next()) {
        QJsonDocument dep = depExport(query.value("begin").toInt(), query.value("end").toInt());
        QJsonDocument map = mapExport();
        outStreamDEP << dep.toJson();
        outStreamMAP << map.toJson();
    }

    /* Close the file */
    outputFileDEP.close();
    outputFileMAP.close();

    return true;
}

int Export::getLastMonthReceiptId()
{
    QSqlDatabase dbc = QSqlDatabase::database("CN");
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT MAX(receiptNum) as maxID FROM receipts WHERE payedBy=4"));
    query.exec();
    if (query.next()) {
        int maxID = query.value("maxID").toInt();
        query.prepare(QString("SELECT payedBy FROM receipts WHERE receiptNum=:id"));
        query.bindValue(":id", maxID +1);
        query.exec();
        if (query.next()) {
             if (query.value("payedBy").toInt() == 8)
                 return maxID +1;
        }
    }
    return -1;
}

bool Export::createBackup()
{
    QrkSettings settings;
    QString directoryname = settings.value("externalDepDirectory", "").toString();

    if (Utils::isDirectoryWritable(directoryname)) {
        QString filename = QDir::toNativeSeparators(directoryname + "/DEP_backup_" + QDateTime::currentDateTime().toString(Qt::ISODate).replace(':',"").replace('-',"") + ".json");
        if (depExport(filename))
            return true;
    }

    return false;
}

bool Export::createBackup(int &counter)
{
    QSqlDatabase dbc = QSqlDatabase::database("CN");
    QSqlQuery query(dbc);

    query.prepare("SELECT value FROM globals WHERE name='DepCounter')");
    query.exec();
    if (query.next()) {
        counter = query.value("value").toInt();
    } else {
        query.prepare("INSERT INTO globals (value) VALUES (3)");
        query.exec();
        counter = 0;
    }

    bool ret = createBackup();
    if (ret) {
        query.prepare("UPDATE globals SET value=:value WHERE name='DepCounter'");
        query.bindValue(":value", (counter == 0)?3:counter -1);
        query.exec();
    }

    return ret;

}
