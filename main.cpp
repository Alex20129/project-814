#include <QCoreApplication>
#include "globals.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    gAppLogger=new Logger;
    gAppLogger->Log("Log begin...", LOG_NOTICE);

    gScanner=new Scanner;
    gDeviceList=new QVector <ASICDevice *>;

    //QCoreApplication::connect(gMainWindow, SIGNAL(need_to_show_scanner_window()), gScanner, SLOT(show()));

    gAppLogger->Log("Everything is prepared, start now.", LOG_NOTICE);

    return a.exec();
}
