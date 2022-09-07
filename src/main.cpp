#include "MainWindow.h"
#include "StartDialog.h"
#include <QErrorMessage>

#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(APPLICATION_NAME);

    QApplication a(argc, argv);
    StartDialog dialog;
    if(dialog.exec() == QDialog::Accepted)
    {
        dialog.updateSettings();
        try {
            MainWindow w(dialog.database.result());
            w.showMaximized();
            return a.exec();
        }  catch (DatabaseException &ex) {
            QErrorMessage errorMessageDialog;
            errorMessageDialog.showMessage(ex.getErrPath());
            errorMessageDialog.exec();
        }
    }
    return 0;
}
