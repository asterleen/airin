#ifndef AIRINCOMMANDS_H
#define AIRINCOMMANDS_H

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include <QProcess>
#include <QCoreApplication>
#include <QTimer>

#include "airinclient.h"
#include "airinlogger.h"
#include "airindata.h"
#include "airindatabase.h"

// This made for interaction with the AirinServer object
class AirinServer;

class AirinCommands : public QObject
{
    Q_OBJECT
public:
    explicit AirinCommands(QObject *parent = 0);

    enum UserCmdResponse
    {
        UCR_INFO,
        UCR_WARNING,
        UCR_ERROR
    };


    static bool process (QString commandLine, AirinClient *client, AirinServer *server);
    static void sendClientResponse(AirinClient *client, QString message,
                                  UserCmdResponse responseType = UCR_INFO);

    // Broadcasts specified message to all clients or selective when 'login' is specified.
    static void serviceBroadcast (AirinServer *server, QString message,
                                  UserCmdResponse responseType = UCR_INFO,
                                  QString login = QString());

    static int parseMessageId (QString id);

    static QStringList getClientStats (AirinServer *server, uint *duplicates = 0);

private:
    static void log (QString message, LogLevel level = LL_DEBUG);

signals:

public slots:
};

#endif // AIRINCOMMANDS_H
