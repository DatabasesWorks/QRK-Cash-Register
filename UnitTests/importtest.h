#ifndef IMPORTTEST_H
#define IMPORTTEST_H

#include <QObject>

class ImportTest
{
public:
    static bool test();

private:
    static int IntRand(int low, int high);
    static double DoubleRand();
    static QString GetRandomString();
    static bool receipt();
    static bool r2b();

};

#endif // IMPORTTEST_H
