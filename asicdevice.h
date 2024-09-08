#ifndef ASICDEVICE_H
#define ASICDEVICE_H

#include <QObject>
#include <QTimer>
#include <QtNetwork>

class ASICDevice : public QObject
{
	Q_OBJECT
signals:
	void DataReceived(ASICDevice *device);
	void DeviceError(ASICDevice *device);
	void DeviceExists(ASICDevice *device);
	void Updated();
public:
	static unsigned int ActiveThreadsNum;
	explicit ASICDevice(QObject *parent=nullptr);
	~ASICDevice();
	void SetAddress(QHostAddress address);
	void SetUserName(QString userName);
	void SetPassword(QString passWord);
	void SetWebPort(quint16 port);
	void SetAPIPort(quint16 port);
	void SetGroupID(uint id);
	void SetUpdateInterval(uint msec);
	void SetNetworkRequestLifetime(uint msec);
	void UploadDataWithPOSTRequest(QString path, QByteArray *DataToSend);
	QHostAddress Address();
	QUrl URL();
	bool IsActive();
	bool IsAlarmed();
	unsigned int NetLag;
	QString Type, Miner;
	QStringList Pools;
public slots:
	void Start();
	void Stop();
private:
	uint pUpdateInterval;
	uint pNetworkRequestLifetime;
	QTimer *pUpdateTimer, *pRequestLifeTimer;
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
private slots:
	void RequestDeviceData();
	void ProcessDeviceData(QNetworkReply *reply);
	void on_AuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
	void on_DataReceived();
	void on_metaDataChanged();
};

#endif // ASICDEVICE_H
