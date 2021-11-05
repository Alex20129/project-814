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
    void DiscoverNetworkInterfaces();
    void SetUserName(QString userName);
    void SetPassword(QString password);
public slots:
    void StartScanning();
    void StopScanning();
    void updateDeviceList(ASICDevice *device);
    void clearUpDeviceList(ASICDevice *device);
private:
    QString pUserName, pPassword;
    QVector <ASICDevice *> UncheckedDevices;
    QList <QNetworkAddressEntry> KnownIFAddresses;
    quint8 pNeedToStopNow, pIsBusy;
private slots:
    void on_ScanIsRun();
    void on_ScanIsDone();
    void on_NewDeviceFound();
};

#endif // SCANNERWINDOW_H
