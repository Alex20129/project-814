#ifndef GLOBALS_H
#define GLOBALS_H

#include <QSysInfo>
#include "logger.h"
#include "scanner.h"

#define PROGRAM_NAME    "asicconf"
#define PROGRAM_VERSION "0.1.5"

#define DEFAULT_UPDATE_INTERVAL			4500
#define DEFAULT_NETWORK_REQUEST_TIMEOUT	2000
#define DEFAULT_SINGLE_SHOT_DELAY		3
#define DEFAULT_THREADS_MAX_NUM			10
#define DEFAULT_API_PORT					4028
#define DEFAULT_WEB_PORT					80

#define UNCHECKED_DEVICES_MAX_NUM		512
#define DEVICE_POOLS_NUM					3

#define NO_ERROR							0
#define ERROR_NETWORK					1
#define ERROR_NETWORK_REQUEST_TIMEOUT	2
#define ERROR_NETWORK_NO_DATA			3

#if defined(Q_OS_LINUX)
    #define DEFAULT_USER_AGENT PROGRAM_NAME "/" PROGRAM_VERSION " (X11; Linux; x86_64)"
#elif defined(Q_OS_WIN)
    #define DEFAULT_USER_AGENT PROGRAM_NAME "/" PROGRAM_VERSION " (Windows NT 6.1; Windows; x86_64)"
#elif defined(Q_OS_MACOS)
    #define DEFAULT_USER_AGENT PROGRAM_NAME "/" PROGRAM_VERSION " (Macintosh; MacOS; x86_64)"
#endif

extern Logger *gAppLogger;
extern Scanner *gScanner;
extern QVector <ASICDevice *> *gKnownDevicesList;

#endif // GLOBALS_H
