#ifndef SCANNERWINDOW_H
#define SCANNERWINDOW_H

#include "asicdevice.h"

class Scanner : public QObject
{
    Q_OBJECT
signals:
    void ScanIsRun();
    void ScanIsDone();
public:
    explicit Scanner(QObject *parent=nullptr);
    QString UserName, Password;
    quint16 APIport, WEBport;
public slots:
    void updateDeviceList(ASICDevice *device);
    void clearUpDeviceList(ASICDevice *device);
private:
    QVector <ASICDevice *> Devices;
private slots:
    void on_scanIsRun();
    void on_scanIsDone();
    void on_scanButton_clicked();
    void on_stopButton_clicked();
};

#endif // SCANNERWINDOW_H