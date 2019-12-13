#ifndef QRKGASTROCURFEWCHECKER_H
#define QRKGASTROCURFEWCHECKER_H

#include <QObject>
#include <QTimer>

class QrkGastroCurfewChecker : public QObject
{
    Q_OBJECT

public:
    QrkGastroCurfewChecker(QObject *parent = Q_NULLPTR);
    ~QrkGastroCurfewChecker();
    void run();

signals:
    void curFew(qint64 diff);
    void finished();

private slots:
    void getCurfewDiff();

private:
    QTimer *m_timer;

};

#endif // QRKGASTROCURFEWCHECKER_H
