#include <QCoreApplication>
#include "globals.h"

int main(int argc, char *argv[])
{
    QCoreApplication App(argc, argv);

    gAppLogger=new Logger;
    gAppLogger->Log("Log begin...", LOG_NOTICE);

    gKnownDevicesList=new QVector <ASICDevice *>;
    gScanner=new Scanner;
    gScanner->SetUserName(QString("root"));
    gScanner->SetPassword(QString("root"));

    gAppLogger->Log("Everything is prepared, start now.", LOG_NOTICE);

    QTimer::singleShot(DEFAULT_SINGLE_SHOT_DELAY, Qt::CoarseTimer, gScanner, SLOT(StartScanning()));

    return App.exec();
}
