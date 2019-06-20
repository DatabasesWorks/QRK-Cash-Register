/*
 * Manipulationssicherheit
 * Die Datenbank für das DEP-7 wird in der Hauptdatenbank mitgeführt, ist allerdings vom User
 * nicht direkt les- oder änderbar.
 * Durch die Kombination aus fortlaufender Sequenznummer und dem Hashwert aus
 * den Zeileneinträgen ist das DEP-7 auch nach dem Export vor Manipulation geschützt.
*/

#include "exportdep.h"
#include "exportdialog.h"
#include <QMessageBox>

ExportDEP::ExportDEP(QObject *parent)
    :Export(parent)
{

}

void ExportDEP::dExport()
{
    ExportDialog dlg(true);
    if (dlg.exec() == QDialog::Accepted ) {
        QString filename = dlg.getFilename();
        if (depExport(filename, dlg.getFrom(), dlg.getTo())) {
            Spread::Instance()->setProgressBarValue(-1);
            QMessageBox::information(Q_NULLPTR, tr("Export"), tr("DEP-7 (Daten-Erfassungs-Protokol) wurde nach %1 exportiert.").arg(filename));
        } else {
            Spread::Instance()->setProgressBarValue(-1);
            QMessageBox::warning(Q_NULLPTR, tr("Export"), tr("DEP-7 (Daten-Erfassungs-Protokol) konnte nicht nach %1 exportiert werden.\nÜberprüfen Sie bitte Ihre Schreibberechtigung.").arg(filename));
        }
    }
}
