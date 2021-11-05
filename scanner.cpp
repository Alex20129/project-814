#include "globals.h"
#include "scanner.h"

Scanner::Scanner(QObject *parent) : QObject(parent)
{
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
    disconnect(device, 0, 0, 0);
    UncheckedDevices.removeOne(device);
    device->Stop();
    device->SetNetworkRequestTimeout(DEFAULT_NETWORK_REQUEST_TIMEOUT);
    device->SetUpdateInterval(DEFAULT_UPDATE_INTERVAL);
    gKnownDevicesList->append(device);
    emit(NewDeviceFound());
    gAppLogger->Log("New device found: "+device->Address().toString());
    if(UncheckedDevices.isEmpty())
    {
        emit(ScanIsDone());
    }
}

void Scanner::clearUpDeviceList(ASICDevice *device)
{
    disconnect(device, 0, 0, 0);
    UncheckedDevices.removeOne(device);
    device->Stop();
    device->deleteLater();
    if(UncheckedDevices.isEmpty())
    {
        emit(ScanIsDone());
    }
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
    gAppLogger->Log("ASICDevice::on_NewDeviceFound()", LOG_DEBUG);
}

void Scanner::StartScanning()
{
    gAppLogger->Log("ASICDevice::StartScanning()", LOG_DEBUG);
    quint32 address;
    QHostAddress AddrFrom, AddrTo;
    emit(ScanIsRun());

    UncheckedDevices.clear();
    foreach(QNetworkAddressEntry IFAddress, KnownIFAddresses)
    {
        uint32_t lastPossible=(uint32_t)(0xFFFFFFFF-IFAddress.netmask().toIPv4Address());
        AddrFrom=QHostAddress((quint32)(IFAddress.ip().toIPv4Address()&IFAddress.netmask().toIPv4Address())+1);
        AddrTo=QHostAddress((quint32)(IFAddress.ip().toIPv4Address()&IFAddress.netmask().toIPv4Address())+lastPossible);
        //gAppLogger->Log("first possible device "+AddrFrom.toString());
        //gAppLogger->Log("last possible device "+AddrTo.toString());
        for(address=AddrFrom.toIPv4Address(); address<AddrTo.toIPv4Address(); address++)
        {
            ASICDevice *newDevice=new ASICDevice;
            newDevice->SetAddress(QHostAddress(address));
            newDevice->SetUserName(this->pUserName);
            newDevice->SetPassword(this->pPassword);
            newDevice->SetAPIPort(this->APIport);
            newDevice->SetWebPort(this->WEBport);
            newDevice->SetNetworkRequestTimeout(300);
            newDevice->SetUpdateInterval(50);
            UncheckedDevices.append(newDevice);
            connect(newDevice, SIGNAL(DeviceExists(ASICDevice *)), this, SLOT(updateDeviceList(ASICDevice *)));
            connect(newDevice, SIGNAL(DeviceError(ASICDevice *)), this, SLOT(clearUpDeviceList(ASICDevice *)));
            while(UncheckedDevices.count()>UNCHECKED_DEVICES_MAX_NUM)
            {
                QCoreApplication::processEvents();
            }
            newDevice->Start();
        }
    }
}

void Scanner::StopScanning()
{
    gAppLogger->Log("ASICDevice::StopScanning()", LOG_DEBUG);
    while(UncheckedDevices.count())
    {
        clearUpDeviceList(UncheckedDevices.last());
    }
}
