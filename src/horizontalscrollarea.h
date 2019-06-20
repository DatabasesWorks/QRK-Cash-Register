#ifndef HORIZONTALSCROLLAREA_H
#define HORIZONTALSCROLLAREA_H

#include <QGridLayout>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>

#include <QDebug>

class HorizontalScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    HorizontalScrollArea(int rows, int cols, QWidget *parent = Q_NULLPTR);

    void addWidget(QWidget *w, int row, int col);
    int columnCount() const;
    void clear();

private:
    void adaptSize();
    QWidget *contentWidget;
    QGridLayout *grid;
    int nRows;
    int nColumns;

protected:
    void resizeEvent(QResizeEvent *event);
};

#endif // HORIZONTALSCROLLAREA_H
