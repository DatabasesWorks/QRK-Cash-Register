#include "pluginview.h"
#include "pluginmanager/treemodel.h"
#include "pluginmanager/Interfaces/barcodesinterface.h"
#include "pluginmanager/Interfaces/wsdlinterface.h"
#include "pluginmanager.h"
#include "ui_pluginview.h"

PluginView::PluginView(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginView)
{
    ui->setupUi(this);

    m_model = new TreeModel(this);
    ui->treeView->setModel(m_model);
    ui->treeView->setWindowTitle(tr("Plugins"));

    connect(ui->treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemDoubleClicked(QModelIndex)));
}

PluginView::~PluginView()
{
    delete ui;
}

void PluginView::itemDoubleClicked(QModelIndex idx)
{
    QModelIndex firstColumnIndex = idx.sibling(idx.row(),0);
    QString name = firstColumnIndex.data().toString();

    QObject *object = PluginManager::instance()->getObjectByName(name);
    if (object) {
        BarcodesInterface *barcodesplugin = qobject_cast<BarcodesInterface *>(object);
        WsdlInterface *wsdlplugin = qobject_cast<WsdlInterface *>(object);
        QDialog *dialog = 0;
        if (barcodesplugin) {
            dialog = barcodesplugin->SettingsDialog();
        } else if (wsdlplugin) {
            dialog = wsdlplugin->SettingsDialog();
        }
        if (dialog)
            dialog->exec();
    }
}
