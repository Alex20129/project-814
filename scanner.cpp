#include "globals.h"
#include "scanner.h"

Scanner::Scanner(QObject *parent) : QObject(parent)
{
    connect(this, SIGNAL(ScanIsDone()), this, SLOT(on_ScanIsDone()));
    connect(this, SIGNAL(ScanIsRun()), this, SLOT(on_ScanIsRun()));

    foreach(QNetworkInterface netInterface, QNetworkInterface::allInterfaces())
    {
        if(!netInterface.isValid())
        {
            continue;
        }
        QNetworkInterface::InterfaceFlags flags = netInterface.flags();
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

    QHostAddress AddrFrom, AddrTo;
    foreach(QNetworkAddressEntry IFAddress, KnownIFAddresses)
    {
        uint32_t lastPossible=(uint32_t) (0xFFFFFFFF-IFAddress.netmask().toIPv4Address()-1);
        AddrFrom=QHostAddress((quint32) (IFAddress.ip().toIPv4Address() & IFAddress.netmask().toIPv4Address())+1);
        AddrTo=QHostAddress((quint32) (IFAddress.ip().toIPv4Address() & IFAddress.netmask().toIPv4Address())+lastPossible);
        gAppLogger->Log("first possible device "+AddrFrom.toString());
        gAppLogger->Log("last possible device "+AddrTo.toString());
    }
}

void Scanner::updateDeviceList(ASICDevice *device)
{
    gAppLogger->Log("Scanner::updateDeviceList()", LOG_DEBUG);
    disconnect(device, 0, 0, 0);
    UncheckedDevices.removeOne(device);
    device->Stop();
    gKnownDevicesList->append(device);
    emit(NewDeviceFound());
    if(UncheckedDevices.isEmpty())
    {
        emit(ScanIsDone());
    }
}

void Scanner::clearUpDeviceList(ASICDevice *device)
{
    gAppLogger->Log("Scanner::clearUpDeviceList()", LOG_DEBUG);
    disconnect(device, 0, 0, 0);
    UncheckedDevices.removeOne(device);
    device->Stop();
    device->Abort();
    device->deleteLater();
    if(UncheckedDevices.isEmpty())
    {
        emit(ScanIsDone());
    }
}

void Scanner::on_ScanIsDone()
{
    gAppLogger->Log("Scanner::on_scanIsDone()", LOG_DEBUG);
}

void Scanner::on_ScanIsRun()
{
    gAppLogger->Log("Scanner::on_scanIsRun()", LOG_DEBUG);
}

void Scanner::StartScanning()
{
    emit(ScanIsRun());
    quint32 address;
    QHostAddress AddrFrom, AddrTo;

    foreach(QNetworkAddressEntry IFAddress, KnownIFAddresses)
    {
        uint32_t lastPossible=(uint32_t)0xFFFFFFFF-IFAddress.netmask().toIPv4Address();
        gAppLogger->Log(QString::number(IFAddress.ip().toIPv4Address()));
        gAppLogger->Log(QString::number(IFAddress.netmask().toIPv4Address()));

        AddrFrom=QHostAddress((quint32)(IFAddress.ip().toIPv4Address() & IFAddress.netmask().toIPv4Address()));
        AddrTo=QHostAddress((quint32)(IFAddress.ip().toIPv4Address() & IFAddress.netmask().toIPv4Address() + lastPossible));
        gAppLogger->Log(QString::number(AddrFrom.toIPv4Address()));
        gAppLogger->Log(QString::number(AddrTo.toIPv4Address()));
        gAppLogger->Log(QString::number(lastPossible));
    }


    UncheckedDevices.clear();
    for(address=AddrFrom.toIPv4Address(); address<=AddrTo.toIPv4Address(); address++)
    {
        ASICDevice *newDevice=new ASICDevice;
        newDevice->SetAddress(QHostAddress(address));
        newDevice->SetUserName(this->UserName);
        newDevice->SetPassword(this->Password);
        newDevice->SetAPIPort(this->APIport);
        newDevice->SetWebPort(this->WEBport);
        UncheckedDevices.append(newDevice);
        connect(newDevice, SIGNAL(DeviceExists(ASICDevice *)), this, SLOT(updateDeviceList(ASICDevice *)));
        connect(newDevice, SIGNAL(DeviceError(ASICDevice *)), this, SLOT(clearUpDeviceList(ASICDevice *)));
        newDevice->Check();
    }
}

void Scanner::StopScanning()
{
    while(UncheckedDevices.count())
    {
        clearUpDeviceList(UncheckedDevices.last());
    }
}
