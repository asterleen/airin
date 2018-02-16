#ifndef AIRINLOGGER_H
#define AIRINLOGGER_H

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

#include <QObject>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <QDateTime>
#include <cstdio>

#include "airindata.h"
#include "airinclient.h"

class AirinLogger : public QObject
{
    Q_OBJECT
public:


    AirinLogger(const QString &file, LogLevel verbosity) :
        QObject(), file(file), verbosity(verbosity)
    {
        admin = NULL;

        if (!file.isEmpty() && file != "stdout")
        {
            logFile.setFileName(file);
            if (!logFile.open(QIODevice::Append))
            {
                printf ("Could not open %s for logs, will write to stdout instead!\n", file.toUtf8().data());
                writeStdout = true;
                return;
            }
                else
            {
                printf ("All my messages will be written to %s\nGood luck, goshujin-sama :3\n",
                        file.toUtf8().data());

                writeStdout = false;

                QTextStream(&logFile) << QString("[*] --- NEW LOG SECTION STARTED [%1] ---\n")
                                         .arg(QDateTime::currentDateTime().toString("dd.MM.yy@hh:mm:ss:zzz"));
            }
        }
        else
            writeStdout = true;
    }

    ~AirinLogger() {

        log ("Logger service stopped.", LL_INFO, "logsv");

        if (!writeStdout)
            logFile.close();
    }


    static AirinLogger *instance;

    bool writeStdout;
    QFile logFile;

    void log(QString message, LogLevel logLevel, QString component);

    AirinClient *adminClient();

    void setAdminClient (AirinClient *client);
    void setAdminLogLevel (LogLevel level);
    void logToAdmin (QString message, LogLevel logLevel);


private:
    QString file;
    LogLevel verbosity;
    LogLevel adminVerbosity;

    AirinClient *admin;

private slots:
    void adminDisconnected();
};

#endif // AIRINLOGGER_H
