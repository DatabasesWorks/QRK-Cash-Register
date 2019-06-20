#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include "qrkcore_global.h"

#include <QTableView>

class QRK_EXPORT TableView : public QTableView
{
    Q_OBJECT

public:
    TableView(QWidget *parent=0);
public slots:
    void showOrUpdateToolTip();
protected:
    bool viewportEvent(QEvent *event);
    void mouseMoveEvent(QMouseEvent *);
private:
    QPoint _lastPosition;
    QPoint _lastGlobalPosition;
    bool _isActive = false;
};

#endif // TABLEVIEW_H
