#include <QtWidgets>
#include <QtDebug>

#include "mainwindow.h"

/**
 * Main function to start GUI
 */
int main(int argv, char *args[])
{
//    Q_INIT_RESOURCE(explorerres);

    QApplication app(argv, args);
    MainWindow mainWindow;
    //mainWindow.setGeometry(100, 100, 1024, 800);
    mainWindow.setGeometry(100, 100, QApplication::desktop()->width()*0.9,
                           QApplication::desktop()->height()*0.9);
    mainWindow.show();

    return app.exec();
}
