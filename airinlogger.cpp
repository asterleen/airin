#include "airinlogger.h"

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

AirinLogger *AirinLogger::instance = 0;

void AirinLogger::log(QString message, LogLevel logLevel, QString component)
{
    if (logLevel > verbosity) // усер выставил чтобы шли только LL_WARNING и критичнее, остальное не выводим
        // Я понял. Ща
        return;

   QString logLevelCode;

   switch (logLevel)
    {
        case LL_DEBUG   : logLevelCode = "DBG"; break;
        case LL_INFO    : logLevelCode = "INF"; break;
        case LL_WARNING : logLevelCode = "WRN"; break;
        case LL_ERROR   : logLevelCode = "ERR"; break;
        case LL_NONE    : return;
    }

   if (writeStdout)
        printf ("[%s] <%s> %s: %s\n",
                QDateTime::currentDateTime().toString("dd.MM.yy@hh:mm:ss:zzz").toUtf8().data(),
                logLevelCode.toUtf8().data(),
                component.toUtf8().data(),
                message.toUtf8().data());
   else
   {
       QTextStream(&logFile) << QString("[%1] <%2> %3: %4\n")
                                .arg(QDateTime::currentDateTime().toString("dd.MM.yy@hh:mm:ss:zzz"))
                                .arg(logLevelCode)
                                .arg(component)
                                .arg(message);

   }

}

AirinClient *AirinLogger::adminClient()
{
    return admin;
}

void AirinLogger::setAdminClient(AirinClient *client)
{
    if (client == NULL)
        return;

    admin = client;
    connect (admin, SIGNAL(disconnected()), this, SLOT(adminDisconnected()));

    log("New admin's client is set, enabling log redirection", LL_INFO, "adlog");
}

void AirinLogger::setAdminLogLevel(LogLevel level)
{
    adminVerbosity = level;
}

void AirinLogger::logToAdmin(QString message, LogLevel logLevel)
{
    if (admin != NULL)
    {
       if (logLevel > adminVerbosity)
            return;

       QString logLevelCode, serviceType;

       switch (logLevel)
        {
            case LL_DEBUG   : logLevelCode = "DBG"; serviceType = "INFO"; break;
            case LL_INFO    : logLevelCode = "INF"; serviceType = "INFO"; break;
            case LL_WARNING : logLevelCode = "WRN"; serviceType = "WARNING"; break;
            case LL_ERROR   : logLevelCode = "ERR"; serviceType = "ERROR"; break;
            case LL_NONE    : return;
        }

       admin->sendMessage(QString("SERVICE %1 #[%2]: %3").arg(serviceType).arg(logLevelCode).arg(message));
    }
}

void AirinLogger::adminDisconnected()
{
    admin = NULL;
    log ("Admin's client has disconnected, disabling log redirect", LL_INFO, "adlog");
}
