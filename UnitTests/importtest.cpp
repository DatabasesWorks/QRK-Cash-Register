#include "importtest.h"
#include "import/importworker.h"
#include "database.h"
#include "databasemanager.h"
#include "preferences/qrksettings.h"
#include "RK/rk_signaturemodule.h"

#include <QDateTime>
#include <QThread>
#include <QTemporaryFile>

int ImportTest::IntRand(int low, int high)
{
    return (qrand() % ((high + low) - low) + low);
}

double ImportTest::DoubleRand() {
    typedef unsigned long long uint64;
    uint64 ret = 0;
    for (int i = 0; i < 13; i++) {
        ret |= ((uint64) (rand() % 16) << i * 4);
    }
    if (ret == 0) {
        return rand() % 2 ? 1.0f : 0.0f;
    }
    uint64 retb = ret;
    unsigned int exp = 0x3ff;
    retb = ret | ((uint64) exp << 52);
    double *tmp = (double*) &retb;
    double retval = *tmp;
    while (retval > 1.0f || retval < 0.0f) {
        retval = *(tmp = (double*) &(retb = ret | ((uint64) (exp--) << 52)));
    }
    if (rand() % 2) {
        retval -= 0.5f;
    }
    return retval;
}

QString ImportTest::GetRandomString()
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    const int randomStringLength = IntRand(5, 15); // assuming you want random strings of 12 characters

    QString randomString;
    for(int i=0; i<randomStringLength; ++i)
    {
        int index = IntRand(0, possibleCharacters.length());
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }
    return randomString;
}


bool ImportTest::receipt()
{
    /*
     * {"receipt":[
     *  {"customertext": "Customer Text",
     *   "payedBy": "0",
     *   "items":[
     *     { "count": "3", "name": "Kupplung", "gross": "122,70", "tax": "20" },
     *     { "count": "1", "name": "Bremsbeläge", "gross": "32,30", "tax": "10" },
     *     { "count": "2", "name": "Benzinschlauch", "gross": "17,80", "tax": "20" },
     *     { "count": "1", "name": "Ölfilter", "gross": "104,50", "tax": "13" }
     *    ]
     *   }
     * ]}
    */


    ImportWorker worker;

    QList<QString> taxList;
    taxList << "0" << "10" << "20";

    QString startJSON =  "{\"receipt\":[\n"
                         "{ \"customertext\": \"Customer Text\",\n"
                         "  \"payedBy\": \"0\",\n"
                         "  \"items\":[\n";
    QString endJSON = "   ]\n"
                      "  }\n"
                      " ]}\n";

    QString docString = "";


    docString.append(startJSON);
    int toCount = IntRand(1, 10);
    for (int count = 0; count < toCount; count++) {
        docString.append("{ \"count\": \"" + QString::number( IntRand(1, 5) ) + "\", \"name\": \"Artikel - " + GetRandomString() + "\", \"gross\": \"" + QString::number(DoubleRand() * 100,'f', 2) + "\", \"tax\": \"" + taxList[(IntRand(0, 3))] + "\" }");
        if (count < toCount-1)
            docString.append(",");
    }
    docString.append(endJSON);

    bool result = worker.processJson(docString.toUtf8(), "dummy.json");

    return result;
}

bool ImportTest::r2b()
{
    /*
     *
     * {"r2b":[
     * {"receiptNum":"RE12345", "net":"22.50", "gross":"27.00", "payedBy":"0", "customerText":"Optionaler Kunden Text" }
     * ]}
     *
     */

    ImportWorker worker;
    QString startJSON =  "{\"r2b\":[\n";
    QString endJSON = " ]}\n";

    QString re = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString docString = "";
    docString.append(startJSON);
    docString.append("{\"receiptNum\":\"RE"  + re.toUpper() + "\", \"gross\":\""  + QString::number(DoubleRand() * 100,'f', 2) +  "\", \"payedBy\":\"0\", \"customerText\":\"Optionaler Kunden Text\" }");
    docString.append(endJSON);

    bool result = worker.processJson(docString.toUtf8(), "dummy.json");

    return result;
}

bool ImportTest::test()
{

    QTemporaryFile tempfile("QRK.db");
    tempfile.open();
    QString filename = tempfile.fileName();
    tempfile.close();
    if (!Database::open(false, filename))
        return false;

    QrkSettings settings;

    settings.save2Database("shopName", "TestCase");
    settings.save2Database("shopCashRegisterId", "Testcase1");
    settings.save2Database("taxlocation", "AT");

    ReceiptItemModel rec;
    if (!rec.createStartReceipt())
        return false;

    for (int n = 0; n < 10; n++) {

        int receiptORr2b = IntRand(0,2);
        if (receiptORr2b == 0) {
            if (!receipt())
                return false;
        } else {
            if (!r2b())
                return false;
        }
        QThread::msleep(200);
    }

    Reports rep(Q_NULLPTR, true);
    bool check = rep.checkEOAny();
    if (check) {
        if (!rep.endOfDay())
            return false;
        if (!rep.endOfMonth())
            return false;
        if (rec.createNullReceipt(CONCLUSION_RECEIPT)) {
            Database::setCashRegisterInAktive();
            RKSignatureModule::setDEPactive(false);
        } else {
            return false;
        }
    } else {
        return false;
    }

    DatabaseManager::removeCurrentThread("CN");
    return true;
}
