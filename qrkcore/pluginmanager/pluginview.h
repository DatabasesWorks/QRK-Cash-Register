#ifndef PLUGINVIEW_H
#define PLUGINVIEW_H

#include "qrkcore_global.h"

#include <QDialog>

#include <QModelIndex>

namespace Ui {
class QRK_EXPORT PluginView;
}

class TreeModel;
class QRK_EXPORT PluginView : public QDialog
{
    Q_OBJECT

public:
    explicit PluginView(QWidget *parent = 0);
    ~PluginView();

private slots:
    void itemDoubleClicked(QModelIndex idx);

private:
    Ui::PluginView *ui;
    TreeModel *m_model;
};

#endif // PLUGINVIEW_H
