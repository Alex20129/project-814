#include "globals.h"
#include "scanner.h"

Scanner::Scanner(QObject *parent) : QObject(parent)
{
	pNeedToStopNow=0;
	pIsBusy=0;
	connect(this, SIGNAL(ScanIsDone()), this, SLOT(on_ScanIsDone()));
	connect(this, SIGNAL(ScanIsRun()), this, SLOT(on_ScanIsRun()));
	connect(this, SIGNAL(NewDeviceFound()), this, SLOT(on_NewDeviceFound()));
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
	gAppLogger->Log("Scanner::on_ScanIsDone()", LOG_DEBUG);
}

void Scanner::on_ScanIsRun()
{
	gAppLogger->Log("Scanner::on_ScanIsRun()", LOG_DEBUG);
}

void Scanner::on_NewDeviceFound()
{
	gAppLogger->Log("Scanner::on_NewDeviceFound() "+gKnownDevicesList->last()->Address().toString(), LOG_DEBUG);
}

void Scanner::StartScanning()
{
	gAppLogger->Log("Scanner::StartScanning()", LOG_DEBUG);
	if(pIsBusy)
	{
		return;
	}
	pIsBusy=1;
	pNeedToStopNow=0;
	emit(ScanIsRun());
	DiscoverNetworkInterfaces();
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
			newDevice->SetNetworkRequestLifetime(300);
			newDevice->SetUpdateInterval(1);
			connect(newDevice, SIGNAL(DeviceExists(ASICDevice *)), this, SLOT(updateDeviceList(ASICDevice *)));
			connect(newDevice, SIGNAL(DeviceError(ASICDevice *)), this, SLOT(clearUpDeviceList(ASICDevice *)));
			UncheckedDevices.append(newDevice);
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
	gAppLogger->Log("Scanner::StopScanning()", LOG_DEBUG);
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

void Scanner::SendNewConfig()
{
	gAppLogger->Log("Scanner::SendNewConfig()", LOG_DEBUG);
	if(gKnownDevicesList->isEmpty())
	{
		gAppLogger->Log("Host list is empty =/\nnothing will be done.", LOG_DEBUG);
		return;
	}
	QStringList NewDeviceSettings;
	QByteArray DataToSend;
	int setting;
	NewDeviceSettings.append(QString("pool1url=stratum+tcp://www.google.com"));
	NewDeviceSettings.append(QString("pool1user=alex"));
	NewDeviceSettings.append(QString("pool1pw=xxx"));
	NewDeviceSettings.append(QString("pool2url=stratum+tcp://www.google.com"));
	NewDeviceSettings.append(QString("pool2user=alex"));
	NewDeviceSettings.append(QString("pool2pw=xxx"));
	NewDeviceSettings.append(QString("pool3url=stratum+tcp://www.google.com"));
	NewDeviceSettings.append(QString("pool3user=alex"));
	NewDeviceSettings.append(QString("pool3pw=xxx"));
	NewDeviceSettings.append(QString("nobeeper=false"));
	NewDeviceSettings.append(QString("notempoverctrl=false"));
	NewDeviceSettings.append(QString("fan_customize_swith=false"));
	NewDeviceSettings.append(QString("fan_customize_value=90"));
	NewDeviceSettings.append(QString("freq=550"));
	NewDeviceSettings.append(QString("voltage=0706"));
	NewDeviceSettings.append(QString("asic_boost=true"));
	foreach(ASICDevice *Device, *gKnownDevicesList)
	{
		DataToSend.clear();
		for(setting=0; setting<NewDeviceSettings.count(); setting++)
		{
			if(NewDeviceSettings.at(setting).split('=').first()==QString("pool1user"))
			{
				DataToSend.append(QString("_ant_pool1user="));
				if(2==NewDeviceSettings.at(setting).split('=', QString::SkipEmptyParts).count())
				{
					DataToSend.append(NewDeviceSettings.at(setting).split('=').last()+
									  QString(".")+
									  QString::number((Device->Address().toIPv4Address()>>24)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address()>>16)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address()>>8)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address())&0xff));
				}
				DataToSend.append(" ");
			}
			else if(NewDeviceSettings.at(setting).split('=').first()==QString("pool2user"))
			{
				DataToSend.append(QString("_ant_pool2user="));
				if(2==NewDeviceSettings.at(setting).split('=', QString::SkipEmptyParts).count())
				{
					DataToSend.append(NewDeviceSettings.at(setting).split('=').last()+
									  QString(".")+
									  QString::number((Device->Address().toIPv4Address()>>24)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address()>>16)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address()>>8)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address())&0xff));
				}
				DataToSend.append(" ");
			}
			else if(NewDeviceSettings.at(setting).split('=').first()==QString("pool3user"))
			{
				DataToSend.append(QString("_ant_pool3user="));
				if(2==NewDeviceSettings.at(setting).split('=', QString::SkipEmptyParts).count())
				{
					DataToSend.append(NewDeviceSettings.at(setting).split('=').last()+
									  QString(".")+
									  QString::number((Device->Address().toIPv4Address()>>24)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address()>>16)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address()>>8)&0xff)+
									  QString("x")+
									  QString::number((Device->Address().toIPv4Address())&0xff));
				}
				DataToSend.append(" ");
			}
			else
			{
				DataToSend.append(QString("_ant_")+NewDeviceSettings.at(setting)+QString(" "));
			}
		}
		Device->UploadDataWithPOSTRequest("/cgi-bin/set_miner_conf.cgi", &DataToSend);
	}
}
