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

    pNetworkRequestTimeout=DEFAULT_NETWORK_REQUEST_TIMEOUT;

    pReceivedData=new QByteArray;
    pAPIManager=new QNetworkAccessManager(this);
    pAPITimer=new QTimer(this);
    pAPITimer->setTimerType(Qt::CoarseTimer);
    pAPITimer->setInterval(DEFAULT_UPDATE_INTERVAL);

    connect(pAPITimer, SIGNAL(timeout()), this, SLOT(RequestDeviceData()));
    connect(pAPIManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(ProcessDeviceData(QNetworkReply *)));
    connect(pAPIManager, SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), this, SLOT(on_AuthenticationRequired(QNetworkReply *, QAuthenticator *)));
    connect(this, SIGNAL(DataReceived(ASICDevice *)), this, SLOT(on_DataReceived()));
}

ASICDevice::~ASICDevice()
{
    delete pReceivedData;
}

void ASICDevice::SetAddress(QHostAddress address)
{
    gAppLogger->Log("ASICDevice::SetAddress() "+address.toString(), LOG_DEBUG);
    pAddress=address;
    pURL.setScheme("http");
    pURL.setHost(pAddress.toString());
    pURL.setPath("/updatecglog.cgi");
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

void ASICDevice::SetNetworkRequestTimeout(uint msec)
{
    pNetworkRequestTimeout=msec;
}

void ASICDevice::SetUpdateInterval(uint msec)
{
    pAPITimer->setInterval(msec);
}

uint ASICDevice::NetworkRequestTimeout()
{
    return pNetworkRequestTimeout;
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
    return(pAPITimer->isActive());
}

bool ASICDevice::IsAlarmed()
{
    return(pLastErrorCode!=NO_ERROR);
}

void ASICDevice::Start()
{
    gAppLogger->Log("ASICDevice::Start()", LOG_DEBUG);
    if(!pAPITimer->isActive())
    {
        pAPITimer->start();
    }
}

void ASICDevice::Stop()
{
    gAppLogger->Log("ASICDevice::Stop()", LOG_DEBUG);
    if(pAPITimer->isActive())
    {
        pAPITimer->stop();
    }
}

void ASICDevice::Abort()
{
    gAppLogger->Log("ASICDevice::Abort()", LOG_DEBUG);
    if(pAPIReply)
    {
        if(pAPIReply->isRunning())
        {
            pAPIReply->abort();
        }
        pAPIReply=nullptr;
    }
}

void ASICDevice::timerEvent(QTimerEvent *event)
{
    if(event->timerId()==pNetworkTimeoutTimerID)
    {
        event->accept();
        pLastErrorCode=ERROR_NETWORK_REQUEST_TIMEOUT;
        this->Abort();
    }
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
            pAPITimer->start();
        }
        ActiveThreadsNum--;
        pIsBusy=false;
        return;
    }
    gAppLogger->Log("ASICDevice::RequestDeviceData() "+pAddress.toString(), LOG_DEBUG);
    QNetworkRequest NewRequest;
    NewRequest.setUrl(pURL);
    NewRequest.setHeader(QNetworkRequest::UserAgentHeader, DEFAULT_USER_AGENT);
    pRequestStartTime=QTime::currentTime();
    pNetworkTimeoutTimerID=this->startTimer(pNetworkRequestTimeout, Qt::CoarseTimer);
    pAPIReply=pAPIManager->get(NewRequest);
    connect(pAPIReply, SIGNAL(metaDataChanged()), this, SLOT(on_metaDataChanged()));
}

void ASICDevice::ProcessDeviceData(QNetworkReply *reply)
{
    gAppLogger->Log("ASICDevice::ProcessDeviceData()", LOG_DEBUG);
    if(pNetworkTimeoutTimerID)
    {
        this->killTimer(pNetworkTimeoutTimerID);
        pNetworkTimeoutTimerID=0;
    }
    NetLag*=0.8;
    NetLag+=pRequestStartTime.msecsTo(QTime::currentTime())*0.2;
    if(reply->error())
    {
        pLastErrorCode=ERROR_NETWORK;
        emit(DeviceError(this));
        gAppLogger->Log(Address().toString()+" ASICDevice::ProcessDeviceData reply: ERROR: "+reply->errorString(), LOG_ERROR);
        goto alldone;
    }
    else
    {
        gAppLogger->Log(Address().toString()+" ASICDevice::ProcessDeviceData reply: OK", LOG_DEBUG);
    }
    if(!reply->isReadable())
    {
        pLastErrorCode=ERROR_NETWORK_NO_DATA;
        emit(DeviceError(this));
        gAppLogger->Log(Address().toString()+"ASICDevice::ProcessDeviceData reply: not readable", LOG_ERROR);
        goto alldone;
    }
    pLastErrorCode=NO_ERROR;
    pReceivedData->clear();
    *pReceivedData=reply->readAll();
    alldone:
    reply->disconnect();
    reply->deleteLater();
    emit(DataReceived(this));
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
    int i, is_updated=0;
    uint uval;
    char str[128], poolsubstr[512];
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
    if(pAPIReply)
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
}
