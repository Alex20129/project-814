#include "globals.h"
#include "asicdevice.h"

unsigned int ASICDevice::ActiveThreadsNum=0;

ASICDevice::ASICDevice(QObject *parent) : QObject(parent)
{
	pIsBusy=false;
	pLastErrorCode=NO_ERROR;
	pWebPort=DEFAULT_WEB_PORT;
	pAPIPort=DEFAULT_API_PORT;
	pAPIReply=nullptr;

	pReceivedData=new QByteArray;
	pAPIManager=new QNetworkAccessManager(this);

	pUpdateTimer=new QTimer(this);
	pUpdateTimer->setTimerType(Qt::CoarseTimer);
	pUpdateTimer->setInterval(DEFAULT_UPDATE_INTERVAL);
	pUpdateInterval=DEFAULT_UPDATE_INTERVAL;

	pRequestLifeTimer=new QTimer(this);
	//pRequestLifeTimer->setTimerType(Qt::CoarseTimer);
	//pRequestLifeTimer->setInterval(DEFAULT_NETWORK_REQUEST_LIFETIME);
	pNetworkRequestLifetime=DEFAULT_NETWORK_REQUEST_LIFETIME;

	connect(pUpdateTimer, SIGNAL(timeout()), this, SLOT(RequestDeviceData()));
	//connect(pRequestLifeTimer, SIGNAL(timeout()), this, SLOT(Abort()));

	connect(pAPIManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(ProcessDeviceData(QNetworkReply *)));
	connect(pAPIManager, SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), this, SLOT(on_AuthenticationRequired(QNetworkReply *, QAuthenticator *)));
	connect(this, SIGNAL(DataReceived(ASICDevice *)), this, SLOT(on_DataReceived()));
}

ASICDevice::~ASICDevice()
{
	delete pReceivedData;
}

// http://x.x.x.x/cgi-bin/minerConfiguration.cgi
// http://x.x.x.x/cgi-bin/minerStatus.cgi
void ASICDevice::SetAddress(QHostAddress address)
{
	pAddress=address;
	pURL.setScheme("http");
	pURL.setHost(pAddress.toString());
	pURL.setPort(pWebPort);
	pURL.setPath("/cgi-bin/minerConfiguration.cgi");
}

void ASICDevice::SetUserName(QString userName)
{
	pUserName=userName;
	pURL.setUserName(userName);
}

void ASICDevice::SetPassword(QString passWord)
{
	pPassWord=passWord;
	pURL.setPassword(passWord);
}

void ASICDevice::SetWebPort(quint16 port)
{
	pWebPort=port;
	pURL.setPort(port);
}

void ASICDevice::SetAPIPort(quint16 port)
{
	pAPIPort=port;
}

void ASICDevice::SetUpdateInterval(uint msec)
{
	pUpdateInterval=msec;
	if(pUpdateInterval==0)
	{
		pUpdateInterval=DEFAULT_UPDATE_INTERVAL;
	}
	pUpdateTimer->setInterval(pUpdateInterval);
}

void ASICDevice::SetNetworkRequestLifetime(uint msec)
{
	pNetworkRequestLifetime=msec;
	if(pNetworkRequestLifetime==0)
	{
		pNetworkRequestLifetime=DEFAULT_NETWORK_REQUEST_LIFETIME;
	}
	pRequestLifeTimer->setInterval(pNetworkRequestLifetime);
}

QHostAddress ASICDevice::Address()
{
	return(pAddress);
}

QUrl ASICDevice::URL()
{
	return(pURL);
}

bool ASICDevice::IsActive()
{
	return(pUpdateTimer->isActive());
}

bool ASICDevice::IsAlarmed()
{
	return(pLastErrorCode!=NO_ERROR);
}

void ASICDevice::Start()
{
	gAppLogger->Log("ASICDevice::Start()", LOG_DEBUG);
	if(!pUpdateTimer->isActive())
	{
		pUpdateTimer->start();
	}
}

void ASICDevice::Stop()
{
	gAppLogger->Log("ASICDevice::Stop()", LOG_DEBUG);
	if(pUpdateTimer->isActive())
	{
		pUpdateTimer->stop();
	}
}

void ASICDevice::UploadDataWithPOSTRequest(QString path, QByteArray *DataToSend)
{
	if(pIsBusy)
	{
		return;
	}
	pIsBusy=true;
	ActiveThreadsNum++;
	if(ActiveThreadsNum>DEFAULT_THREADS_MAX_NUM)
	{
		if(this->IsActive())
		{
			pUpdateTimer->start();
		}
		ActiveThreadsNum--;
		pIsBusy=false;
		return;
	}
	gAppLogger->Log("ASICDevice::UploadDataWithPOSTRequest()\n"+pURL.toString(), LOG_DEBUG);
	QUrl deviceURL;
	deviceURL.setScheme("http");
	deviceURL.setHost(this->pAddress.toString());
	deviceURL.setPort(this->pWebPort);
	deviceURL.setUserName(this->pUserName);
	deviceURL.setPassword(this->pPassWord);
	deviceURL.setPath(path);
	QNetworkRequest setingsRequest;
	setingsRequest.setUrl(deviceURL);
	setingsRequest.setHeader(QNetworkRequest::UserAgentHeader, DEFAULT_USER_AGENT);
	setingsRequest.setHeader(QNetworkRequest::ContentTypeHeader, DEFAULT_CONTENT_TYPE);
	gAppLogger->Log("now perform the POST request on\n"+deviceURL.toString()+"\nThese data will be sent:\n"+QString(DataToSend->toStdString().data()), LOG_DEBUG);
	pAPIManager->post(setingsRequest, *DataToSend);
}

void ASICDevice::RequestDeviceData()
{
	if(pIsBusy)
	{
		return;
	}
	pIsBusy=true;
	ActiveThreadsNum++;
	if(ActiveThreadsNum>DEFAULT_THREADS_MAX_NUM)
	{
		if(this->IsActive())
		{
			pUpdateTimer->start();
		}
		ActiveThreadsNum--;
		pIsBusy=false;
		return;
	}
	gAppLogger->Log("ASICDevice::RequestDeviceData()\n"+pURL.toString(), LOG_DEBUG);
	QNetworkRequest NewRequest;
	NewRequest.setUrl(pURL);
	NewRequest.setHeader(QNetworkRequest::UserAgentHeader, DEFAULT_USER_AGENT);
	pRequestStartTime=QTime::currentTime();
	pAPIReply=pAPIManager->get(NewRequest);
	connect(pAPIReply, SIGNAL(metaDataChanged()), this, SLOT(on_metaDataChanged()));
	pRequestLifeTimer->singleShot(pNetworkRequestLifetime, pAPIReply, SLOT(abort()));
}

void ASICDevice::ProcessDeviceData(QNetworkReply *reply)
{
	gAppLogger->Log("ASICDevice::ProcessDeviceData()", LOG_DEBUG);
	pRequestLifeTimer->stop();
	if(reply->error())
	{
		pLastErrorCode=ERROR_NETWORK;
		emit(DeviceError(this));
		gAppLogger->Log("\n"+reply->url().toString()+" | ERROR:\n"+reply->errorString(), LOG_ERROR);
		goto alldone;
	}
	else
	{
		gAppLogger->Log("\n"+reply->url().toString()+" | OK", LOG_DEBUG);
	}
	NetLag*=0.8;
	NetLag+=pRequestStartTime.msecsTo(QTime::currentTime())*0.2;
	if(!reply->isReadable())
	{
		pLastErrorCode=ERROR_NETWORK_NO_DATA;
		emit(DeviceError(this));
		gAppLogger->Log(Address().toString()+"\nReply: have no data to read", LOG_ERROR);
		goto alldone;
	}
	pLastErrorCode=NO_ERROR;
	pReceivedData->clear();
	*pReceivedData=reply->readAll();
	emit(DataReceived(this));
	alldone:
	pAPIReply=nullptr;
	reply->disconnect();
	reply->deleteLater();
	ActiveThreadsNum--;
	pIsBusy=false;
}

void ASICDevice::on_AuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
	gAppLogger->Log("ASICDevice::on_AuthenticationRequired()", LOG_DEBUG);
	if(reply->error()==QNetworkReply::NoError)
	{
		gAppLogger->Log(this->Address().toString()+" reply [success]", LOG_DEBUG);
	}
	else
	{
		gAppLogger->Log(this->Address().toString()+" reply [error] "+reply->errorString(), LOG_ERROR);
	}
	authenticator->setPassword(pPassWord);
	authenticator->setUser(pUserName);
}

void ASICDevice::on_DataReceived()
{
	gAppLogger->Log("ASICDevice::on_DataReceived()", LOG_DEBUG);
	int i, is_updated=0;
	uint uval;
	char str[128], poolsubstr[512];
	*pReceivedData=QByteArray("\n")+*pReceivedData;
	gAppLogger->Log(pReceivedData, LOG_DEBUG);
	for(i=0; i<pReceivedData->size(); i++)
	{
		if(1==sscanf(&pReceivedData->data()[i], ",Type=%127[^,|]", str))
		{
			Type=QString(str);
			is_updated++;
			continue;
		}
		if(1==sscanf(&pReceivedData->data()[i], ",Miner=%127[^,|]", str))
		{
			Miner=QString(str);
			is_updated++;
			continue;
		}
		if(2==sscanf(&pReceivedData->data()[i], "|POOL=%u,%511[^|]", &uval, poolsubstr))
		{
			if(uval>=DEVICE_POOLS_NUM)
			{
				continue;
			}
			if(sscanf(poolsubstr, "URL=%127[^,]", str))
			{
				Pools.append(str);
				is_updated++;
			}
			continue;
		}
	}
	if(is_updated)
	{
		emit(Updated());
	}
}

void ASICDevice::on_metaDataChanged()
{
	if(pAPIReply->error())
	{
		pLastErrorCode=ERROR_NETWORK;
		emit(DeviceError(this));
	}
	else
	{
		pLastErrorCode=NO_ERROR;
		emit(DeviceExists(this));
	}
}
