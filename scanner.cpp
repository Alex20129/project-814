#include "globals.h"
#include "scanner.h"

Scanner::Scanner(QObject *parent) : QObject(parent)
{
    pNeedToStopNow=0;
    pIsBusy=0;
    connect(this, SIGNAL(ScanIsDone()), this, SLOT(on_ScanIsDone()));
    connect(this, SIGNAL(ScanIsRun()), this, SLOT(on_ScanIsRun()));
    connect(this, SIGNAL(NewDeviceFound()), this, SLOT(on_NewDeviceFound()));
    DiscoverNetworkInterfaces();
}

void Scanner::DiscoverNetworkInterfaces()
{
    gAppLogger->Log("Scanner::DiscoverNetworkInterfaces()", LOG_DEBUG);
    foreach(QNetworkInterface netInterface, QNetworkInterface::allInterfaces())
    {
        if(!netInterface.isValid())
        {
            continue;
        }
        QNetworkInterface::InterfaceFlags flags=netInterface.flags();
        if(flags.testFlag(QNetworkInterface::IsRunning) && !flags.testFlag(QNetworkInterface::IsLoopBack))
        {
            gAppLogger->Log("Device: " + netInterface.name(), LOG_NOTICE);
            gAppLogger->Log("HardwareAddress: " + netInterface.hardwareAddress(), LOG_NOTICE);

            foreach(QNetworkAddressEntry addrEntry, netInterface.addressEntries())
            {
                if(addrEntry.ip().protocol()!=QAbstractSocket::IPv6Protocol &&
                   !addrEntry.ip().isLoopback() && !addrEntry.ip().isNull())
                {
                    KnownIFAddresses.append(addrEntry);
                    gAppLogger->Log("IP Address: " + addrEntry.ip().toString(), LOG_NOTICE);
                    gAppLogger->Log("Netmask: " + addrEntry.netmask().toString(), LOG_NOTICE);
                    gAppLogger->Log("Broadcast: " + addrEntry.broadcast().toString(), LOG_NOTICE);
                }
            }
        }
    }
}

void Scanner::SetUserName(QString userName)
{
    pUserName=userName;
}

void Scanner::SetPassword(QString password)
{
    pPassword=password;
}

void Scanner::updateDeviceList(ASICDevice *device)
{
    disconnect(device, 0, this, 0);
    UncheckedDevices.removeOne(device);
    device->Stop();
    device->SetNetworkRequestLifetime(DEFAULT_NETWORK_REQUEST_LIFETIME);
    device->SetUpdateInterval(DEFAULT_UPDATE_INTERVAL);
    gKnownDevicesList->append(device);
    emit(NewDeviceFound());
}

void Scanner::clearUpDeviceList(ASICDevice *device)
{
    disconnect(device, 0, this, 0);
    UncheckedDevices.removeOne(device);
    device->Stop();
    device->deleteLater();
}

void Scanner::on_ScanIsDone()
{
    gAppLogger->Log("ASICDevice::on_ScanIsDone()", LOG_DEBUG);
}

void Scanner::on_ScanIsRun()
{
    gAppLogger->Log("ASICDevice::on_ScanIsRun()", LOG_DEBUG);
}

void Scanner::on_NewDeviceFound()
{
    gAppLogger->Log("ASICDevice::on_NewDeviceFound() "+gKnownDevicesList->last()->Address().toString(), LOG_DEBUG);
}

void Scanner::StartScanning()
{
    gAppLogger->Log("ASICDevice::StartScanning()", LOG_DEBUG);
    if(pIsBusy)
    {
        return;
    }
    pIsBusy=1;
    pNeedToStopNow=0;
    emit(ScanIsRun());
    quint32 address, lastPossible;
    QHostAddress AddrFrom, AddrTo;
    foreach(QNetworkAddressEntry IFAddress, KnownIFAddresses)
    {
        lastPossible=(quint32)(0xFFFFFFFF-IFAddress.netmask().toIPv4Address());
        AddrFrom=QHostAddress((quint32)(IFAddress.ip().toIPv4Address()&IFAddress.netmask().toIPv4Address())+1);
        AddrTo=QHostAddress((quint32)(IFAddress.ip().toIPv4Address()&IFAddress.netmask().toIPv4Address())+lastPossible);
        //gAppLogger->Log("first possible device "+AddrFrom.toString());
        //gAppLogger->Log("last possible device "+AddrTo.toString());
        for(address=AddrFrom.toIPv4Address(); address<AddrTo.toIPv4Address() && !pNeedToStopNow; address++)
        {
            ASICDevice *newDevice=new ASICDevice;
            newDevice->SetAddress(QHostAddress(address));
            newDevice->SetUserName(this->pUserName);
            newDevice->SetPassword(this->pPassword);
            newDevice->SetAPIPort(this->APIport);
            newDevice->SetWebPort(this->WEBport);
            newDevice->SetNetworkRequestLifetime(300);
            newDevice->SetUpdateInterval(50);
            UncheckedDevices.append(newDevice);
            connect(newDevice, SIGNAL(DeviceExists(ASICDevice *)), this, SLOT(updateDeviceList(ASICDevice *)));
            connect(newDevice, SIGNAL(DeviceError(ASICDevice *)), this, SLOT(clearUpDeviceList(ASICDevice *)));
            newDevice->Start();
            while(UncheckedDevices.count()>UNCHECKED_DEVICES_MAX_NUM)
            {
                QCoreApplication::processEvents();
            }
        }
    }
    while(UncheckedDevices.count())
    {
        QCoreApplication::processEvents();
    }
    emit(ScanIsDone());
    pNeedToStopNow=0;
    pIsBusy=0;
}

void Scanner::StopScanning()
{
    gAppLogger->Log("ASICDevice::StopScanning()", LOG_DEBUG);
    if(pNeedToStopNow)
    {
        return;
    }
    pNeedToStopNow=1;
    ASICDevice *Device;
    while(UncheckedDevices.count())
    {
        Device=UncheckedDevices.takeLast();
        disconnect(Device, 0, 0, 0);
        Device->Stop();
        Device->deleteLater();
    }
}
