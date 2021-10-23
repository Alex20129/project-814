#include "globals.h"
#include "scanner.h"

Scanner::Scanner(QObject *parent) : QObject(parent)
{
    connect(this, SIGNAL(ScanIsDone()), this, SLOT(on_scanIsDone()));
    connect(this, SIGNAL(ScanIsRun()), this, SLOT(on_scanIsRun()));

    for(int i=0; i<QNetworkInterface::allAddresses().count(); i++)
    {
        QHostAddress addr=QNetworkInterface::allAddresses().at(i);
        if(!addr.isLoopback() && !addr.isNull())
        {
            gAppLogger->Log(addr.toString()+" interface address discovered", LOG_NOTICE);
        }
    }
}

void Scanner::updateDeviceList(ASICDevice *device)
{
    gAppLogger->Log("Scanner::updateDeviceList()", LOG_DEBUG);
    disconnect(device, nullptr, this, nullptr);
    Devices.removeOne(device);
    device->Stop();
    device->SetNetworkRequestTimeout(5000);
    if(Devices.isEmpty())
    {
        emit(ScanIsDone());
    }
}

void Scanner::clearUpDeviceList(ASICDevice *device)
{
    gAppLogger->Log("Scanner::clearUpDeviceList()", LOG_DEBUG);
    disconnect(device, 0, 0, 0);
    Devices.removeOne(device);
    device->Stop();
    device->Abort();
    device->deleteLater();
    if(Devices.isEmpty())
    {
        emit(ScanIsDone());
    }
}

void Scanner::on_scanIsDone()
{
    gAppLogger->Log("Scanner::on_scanIsDone()", LOG_DEBUG);
}

void Scanner::on_scanIsRun()
{
    gAppLogger->Log("Scanner::on_scanIsRun()", LOG_DEBUG);
}

void Scanner::on_scanButton_clicked()
{
    emit(ScanIsRun());
    quint32 address;
    QHostAddress AddrFrom((quint32)(0)), AddrTo((quint32)(10));
    Devices.clear();
    for(address=AddrFrom.toIPv4Address(); address<=AddrTo.toIPv4Address(); address++)
    {
        ASICDevice *newDevice=new ASICDevice;
        newDevice->SetAddress(QHostAddress(address));
        newDevice->SetUserName(this->UserName);
        newDevice->SetPassword(this->Password);
        newDevice->SetAPIPort(this->APIport);
        newDevice->SetWebPort(this->WEBport);
        Devices.append(newDevice);
        connect(newDevice, SIGNAL(DeviceExists(ASICDevice *)), this, SLOT(updateDeviceList(ASICDevice *)));
        connect(newDevice, SIGNAL(DeviceError(ASICDevice *)), this, SLOT(clearUpDeviceList(ASICDevice *)));
        newDevice->Check();
    }
}

void Scanner::on_stopButton_clicked()
{
    while(Devices.count())
    {
        clearUpDeviceList(Devices.last());
    }
}
