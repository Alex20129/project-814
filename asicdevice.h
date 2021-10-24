#ifndef ASICDEVICE_H
#define ASICDEVICE_H

#include <QObject>
#include <QTimer>
#include <QtNetwork>

class ASICDevice : public QObject
{
    Q_OBJECT
public:
    static unsigned int ActiveThreadsNum;
    explicit ASICDevice(QObject *parent=nullptr);
    void SetAddress(QHostAddress address);
    void SetUserName(QString userName);
    void SetPassword(QString passWord);
    void SetWebPort(quint16 port);
    void SetAPIPort(quint16 port);
    void SetGroupID(uint id);
    void SetNetworkRequestTimeout(uint timeout);
    uint NetworkRequestTimeout();
    double THSmm;
    double THSavg;
    double Freq;
    int MTmax[3];
    uint MTavg[3];
    uint MW[3];
    uint Temp;
    uint TMax;
    uint Fan[4];
    uint FanMax[4];
    QHostAddress Address();
    QUrl URL();
    uint GroupID();
    bool IsActive();
    bool IsAlarmed();
    unsigned int NetLag;
signals:
    void DataReceived(ASICDevice *device);
    void DeviceError(ASICDevice *device);
    void DeviceExists(ASICDevice *device);
    void Updated();
public slots:
    void Start();
    void Check();
    void Stop();
    void Abort();
private:
    uint pNetworkRequestTimeout;
    QTimer *pAPITimer;
    QNetworkAccessManager *pAPIManager;
    QNetworkReply *pAPIReply;
    bool pIsBusy;
    uint pLastErrorCode;
    QHostAddress pAddress;
    QString pUserName, pPassWord;
    quint16 pWebPort, pAPIPort;
    uint pGroupID;
    QUrl pURL;
    QTime pRequestStartTime;
    QByteArray *pReceivedData;
    int pNetworkTimeoutTimerID;
    void timerEvent(QTimerEvent *event);
private slots:
    void RequestDeviceData();
    void ProcessDeviceData(QNetworkReply *reply);
    void on_AuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void on_DataReceived();
    void on_metaDataChanged();
};

#endif // ASICDEVICE_H
