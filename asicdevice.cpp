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
    pGroupID=
    THSmm=
    THSavg=
    Freq=
    WORKMODE=
    MTmax[0]=
    MTmax[1]=
    MTmax[2]=
    MTavg[0]=
    MTavg[1]=
    MTavg[2]=
    MW[0]=
    MW[1]=
    MW[2]=
    Temp=
    TMax=
    Fan[0]=
    Fan[1]=
    Fan[2]=
    Fan[3]=
    FanMax[0]=
    FanMax[1]=
    FanMax[2]=
    FanMax[3]=0;

    pNetworkRequestTimeout=DEFAULT_NETWORK_REQUEST_TIMEOUT;

    pReceivedData=new QByteArray;
    pAPITimer=new QTimer(this);
    pAPIManager=new QNetworkAccessManager(this);
    pAPITimer->setInterval(DEFAULT_UPDATE_INTERVAL);

    connect(pAPITimer, SIGNAL(timeout()), this, SLOT(RequestDeviceData()));
    connect(pAPIManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(ProcessDeviceData(QNetworkReply *)));
    connect(pAPIManager, SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), this, SLOT(on_AuthenticationRequired(QNetworkReply *, QAuthenticator *)));

    connect(this, SIGNAL(DataReceived(ASICDevice *)), this, SLOT(on_DataReceived()));
    connect(this, SIGNAL(Updated()), this, SLOT(Refresh()));
}

void ASICDevice::SetAddress(QHostAddress address)
{
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

void ASICDevice::SetGroupID(uint id)
{
    pGroupID=id;
}

void ASICDevice::SetNetworkRequestTimeout(uint timeout)
{
    pNetworkRequestTimeout=timeout;
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

uint ASICDevice::GroupID()
{
    return(pGroupID);
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
    if(!pAPITimer->isActive())
    {
        pAPITimer->start();
    }
}

void ASICDevice::Check()
{
    if(!pAPITimer->isActive())
    {
        pAPITimer->singleShot(3, this, SLOT(RequestDeviceData()));
    }
}

void ASICDevice::Stop()
{
    if(pAPITimer->isActive())
    {
        pAPITimer->stop();
    }
}

void ASICDevice::Abort()
{
    if(pAPIReply)
    {
        if(pAPIReply->error())
        {
            pAPIReply->disconnect();
            pAPIReply->deleteLater();
        }
        else if(pAPIReply->isRunning())
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
        this->killTimer(pNetworkTimeoutTimerID);
        event->accept();
        pNetworkTimeoutTimerID=0;
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
        pAPITimer->start();
        ActiveThreadsNum--;
        pIsBusy=false;
        return;
    }
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
    NetLag*=0.8;
    NetLag+=pRequestStartTime.msecsTo(QTime::currentTime())*0.2;
    if(pNetworkTimeoutTimerID)
    {
        this->killTimer(pNetworkTimeoutTimerID);
        pNetworkTimeoutTimerID=0;
    }
    if(reply->error())
    {
        pLastErrorCode=ERROR_NETWORK;
        emit(DeviceError(this));
        qCritical()<<Address().toString()<<"ASICDevice::ProcessDeviceData reply: ERROR:"<<reply->errorString();
        goto alldone;
    }
    else
    {
        //qInfo()<<Address().toString()<<"ASICDevice::ProcessDeviceData reply: OK";
    }
    if(!reply->isReadable())
    {
        pLastErrorCode=ERROR_NETWORK_NO_DATA;
        emit(DeviceError(this));
        qCritical()<<Address().toString()<<"ASICDevice::ProcessDeviceData reply: not readable";
        goto alldone;
    }
    pLastErrorCode=NO_ERROR;
    pReceivedData->clear();
    *pReceivedData=reply->readAll();
    pReceivedData->remove(0, pReceivedData->indexOf(QByteArray("CgLogCallback")));
    pReceivedData->remove(0, pReceivedData->indexOf('{'));
    pReceivedData->remove(pReceivedData->lastIndexOf('}')+1, pReceivedData->length());
    emit(DataReceived(this));
    alldone:
    reply->disconnect();
    reply->deleteLater();
    ActiveThreadsNum--;
    pIsBusy=false;
}

void ASICDevice::on_AuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    if(reply->error()==QNetworkReply::NoError)
    {
        //gAppLogger->Log("ASICDevice::onAuthenticationNeeded reply success");
    }
    else
    {
        //gAppLogger->Log("ASICDevice::onAuthenticationNeeded reply error");
        //gAppLogger->Log(reply->errorString());
    }
    authenticator->setPassword(pPassWord);
    authenticator->setUser(pUserName);
}

void ASICDevice::on_DataReceived()
{
    int i=0, is_updated=0;
    for(; i<pReceivedData->size(); i++)
    {
        if(1==sscanf(&pReceivedData->data()[i], "GHSmm[%lf]", &THSmm))
        {
            unsigned int THSmmRound=THSmm/100.0;
            THSmm=THSmmRound/10.0;
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "GHSavg[%lf]", &THSavg))
        {
            unsigned int THSavgRound=THSavg/100.0;
            THSavg=THSavgRound/10.0;
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "Freq[%lf]", &Freq))
        {
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "WORKMODE[%u]", &WORKMODE))
        {
            is_updated++;
            continue;
        }
        if(3==sscanf(&pReceivedData->data()[i], "MTmax[%i %i %i]", &MTmax[0], &MTmax[1], &MTmax[2]))
        {
            is_updated++;
            continue;
        }
        if(3==sscanf(&pReceivedData->data()[i], "MTavg[%u %u %u]", &MTavg[0], &MTavg[1], &MTavg[2]))
        {
            is_updated++;
            continue;
        }
        if(3==sscanf(&pReceivedData->data()[i], "MW[%u %u %u]", &MW[0], &MW[1], &MW[2]))
        {
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "Temp[%u]", &Temp))
        {
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "TMax[%u]", &TMax))
        {
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "Fan1[%u]", &Fan[0]))
        {
            if(Fan[0]>FanMax[0])
            {
                FanMax[0]=Fan[0];
            }
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "Fan2[%u]", &Fan[1]))
        {
            if(Fan[1]>FanMax[1])
            {
                FanMax[1]=Fan[1];
            }
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "Fan3[%u]", &Fan[2]))
        {
            if(Fan[2]>FanMax[2])
            {
                FanMax[2]=Fan[2];
            }
            is_updated++;
            continue;
        }
        if(1==sscanf(&pReceivedData->data()[i], "Fan4[%u]", &Fan[3]))
        {
            if(Fan[3]>FanMax[3])
            {
                FanMax[3]=Fan[3];
            }
            is_updated++;
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
