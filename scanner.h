#ifndef SCANNERWINDOW_H
#define SCANNERWINDOW_H

#include "asicdevice.h"

class Scanner : public QObject
{
    Q_OBJECT
signals:
    void ScanIsRun();
    void ScanIsDone();
    void NewDeviceFound();
public:
    explicit Scanner(QObject *parent=nullptr);
    QString UserName, Password;
    quint16 APIport, WEBport;
public slots:
    void StartScanning();
    void StopScanning();
    void updateDeviceList(ASICDevice *device);
    void clearUpDeviceList(ASICDevice *device);
private:
    QVector <ASICDevice *> UncheckedDevices;
    QList <QNetworkAddressEntry> AvailableIFAddresses;
private slots:
    void on_ScanIsRun();
    void on_ScanIsDone();
};

#endif // SCANNERWINDOW_H
