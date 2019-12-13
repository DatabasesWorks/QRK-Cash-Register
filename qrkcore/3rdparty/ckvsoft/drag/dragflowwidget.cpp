#include "dragflowwidget.h"
#include "flowlayout.h"
#include "dragpushbutton.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QRegularExpression>
#include <QDrag>
#include <QPainter>
#include <QButtonGroup>

DragFlowWidget::DragFlowWidget(QWidget *parent) : QWidget(parent)
{
    m_flowlayout = new FlowLayout(this, 0,0,0);
    m_buttonGroup = new QButtonGroup(this);
    setAcceptDrops(true);

    connect(m_buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &DragFlowWidget::buttonClicked);
}

DragFlowWidget::~DragFlowWidget()
{
    if (m_flowlayout)
        m_flowlayout->deleteLater();
}

void DragFlowWidget::addWidget(DragPushButton *button)
{
    m_flowlayout->addWidget(button);
    m_buttonGroup->addButton(button, button->getId());
    connect(button, &DragPushButton::mouseLongPressEvent, this, &DragFlowWidget::mouseLongPressEvent);
}

DragPushButton *DragFlowWidget::getDragPushButton(int id)
{
    if (m_flowlayout == Q_NULLPTR)
        return Q_NULLPTR;

    for (int i = 0; i < m_flowlayout->count(); ++i) {
        QWidget *w = m_flowlayout->itemAt(i)->widget();
        if(w != Q_NULLPTR) {
            DragPushButton *pb = static_cast<DragPushButton*>(w);
            if (pb != Q_NULLPTR) {
                if (pb->getId() == id)
                    return pb;
            }
        }
    }
    return Q_NULLPTR;
}

void DragFlowWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(dragflowMimeType()))
        event->acceptProposedAction();
}

void DragFlowWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QWidget *current_children = childAt(event->pos());
    DragPushButton *current_button = qobject_cast<DragPushButton*>(current_children);
    if(current_button){
        if (!current_button->isFixedButton() && event->mimeData()->hasFormat(dragflowMimeType())) {
            hover(event->pos());
            if (children().contains(event->source())) {
                event->setDropAction(Qt::MoveAction);
                event->accept();
            } else {
                event->acceptProposedAction();
            }
            return;
        }
    }
    hover(event->pos(), true);
    event->ignore();
}

void DragFlowWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(dragflowMimeType())) {
        hover(event->pos(),true);
        QByteArray byteArray = event->mimeData()->data(dragflowMimeType());
        QWidget *widget = *reinterpret_cast<QWidget**>(byteArray.data());
        DragPushButton *new_button =  qobject_cast<DragPushButton*>(widget);

        QWidget *current_children = childAt(event->pos());
        DragPushButton *current_button = qobject_cast<DragPushButton*>(current_children);
        if(new_button){
            int index = 0;
            if(current_button)
                index = m_flowlayout->indexOf(current_button);
            else{
                index = 0;
                QLayoutItem *item = m_flowlayout->itemAt(index);
                while(item && item->widget()->pos().y() < event->pos().y())
                    item = m_flowlayout->itemAt(index++);
            }
            m_flowlayout->insertWidget(index, new_button);
            emit orderChanged(indexList());
        } else {
            event->ignore();
        }
    } else {
        event->ignore();
    }
}

void DragFlowWidget::mouseLongPressEvent(QPoint pos)
{
    QWidget *child = childAt(pos);
    if(child) {
        createDrag(pos, child);
    }
}

void DragFlowWidget::createDrag(const QPoint &pos, QWidget *widget)
{
    if(widget == Q_NULLPTR)
        return;

    QByteArray byteArray(reinterpret_cast<char*>(&widget),sizeof(QWidget*));
    QDrag *drag = new QDrag(this);
    QMimeData * mimeData = new QMimeData;
    mimeData->setData(dragflowMimeType(),byteArray);
    drag->setMimeData(mimeData);
    QPoint globalPos = mapToGlobal(pos);
    QPoint p = widget->mapFromGlobal(globalPos);
    drag->setHotSpot(p + QPoint(widget->width() / 2, widget->height()/2));
    drag->setPixmap(widget->grab());
    //widget->hide();
    widget->setEnabled(false);
    if (drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::CopyAction) == Qt::MoveAction) {
        widget->close();
    } else {
        // widget->show();
        widget->setEnabled(true);
    }
}

void DragFlowWidget::hover(QPoint pos, bool drop)
{
    if (m_current) {
        m_current->setStyleSheet(m_stylesheet);
    }

    if (drop)
        return;

    m_current = childAt(pos);
    if (m_current) {
        m_stylesheet = m_current->styleSheet();
        m_current->setStyleSheet("background-color: #55ff7f;");
    }
}

QList<int> DragFlowWidget::indexList()
{
    QList<int> idx;
    DragPushButton *b;

    for (int i = 0; i < m_flowlayout->count(); i++) {
        b = qobject_cast<DragPushButton*>(m_flowlayout->itemAt(i)->widget());
        int id = b->getId();
        if (id != 0)
            idx.append(id);
    }

    return idx;
}
