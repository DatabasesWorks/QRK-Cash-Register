#include "tableview.h"

#include <QToolTip>
#include <QEvent>
#include <QHelpEvent>

TableView::TableView(QWidget* parent)
    : QTableView(parent)
{
    this->setMouseTracking(true);
}

void TableView::showOrUpdateToolTip()
{
    if (QTableView::underMouse() && _isActive) {
        const QModelIndex index = QTableView::indexAt(_lastPosition);
        if (index.isValid()) {
            const QString toolTip = model()->data(model()->index(index.row(), 0), Qt::ToolTipRole).toString();
            QToolTip::showText(_lastGlobalPosition, toolTip, this);
        }
    }
}

void TableView::mouseMoveEvent(QMouseEvent * event)
{
    _isActive = false;
    QToolTip::hideText();

    QTableView::mouseMoveEvent(event);
}

bool TableView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        _lastPosition = static_cast<QHelpEvent*>(event)->pos();
        _lastGlobalPosition = static_cast<QHelpEvent*>(event)->globalPos();
        _isActive = true;
        showOrUpdateToolTip();
        return true;
    }
    return QTableView::viewportEvent(event);
}
