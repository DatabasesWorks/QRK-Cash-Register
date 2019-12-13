#ifndef DRAGFLOWWIDGET_H
#define DRAGFLOWWIDGET_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"
#include "3rdparty/ckvsoft/flowlayout.h"

#include <QWidget>

class QDragEnterEvent;
class QDropEvent;
class DragPushButton;
class QButtonGroup;

class CKVSOFT_EXPORT DragFlowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DragFlowWidget(QWidget *parent = Q_NULLPTR);
    ~DragFlowWidget() override;
    void addWidget(DragPushButton *button);
    DragPushButton *getDragPushButton(int id);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    void buttonClicked(int id);
    void orderChanged(QList<int> indexList);

private:

    static QString dragflowMimeType() { return QStringLiteral("application/x-dragflow"); }
    void mouseLongPressEvent(QPoint pos);
    void hover(QPoint pos, bool drop = false);
    void createDrag(const QPoint &pos, QWidget *widget);
    QList<int> indexList();

    FlowLayout *m_flowlayout = Q_NULLPTR;
    QButtonGroup *m_buttonGroup = Q_NULLPTR;
    QWidget *m_current = Q_NULLPTR;
    QString m_stylesheet;

};

#endif // QRKWIDGET_H
