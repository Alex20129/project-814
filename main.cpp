#include <QCoreApplication>
#include "globals.h"

int main(int argc, char *argv[])
{
    QCoreApplication App(argc, argv);

    gAppLogger=new Logger;
    gAppLogger->Log("Log begin...", LOG_NOTICE);

    gKnownDevicesList=new QVector <ASICDevice *>;
    gScanner=new Scanner;
    gScanner->UserName=QString("root");
    gScanner->Password=QString("root");

    gAppLogger->Log("Everything is prepared, start now.", LOG_NOTICE);

    QTimer::singleShot(3, Qt::CoarseTimer, gScanner, SLOT(StartScanning()));

    return App.exec();
}
