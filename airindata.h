#ifndef AIRINDATA_H
#define AIRINDATA_H

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

#include <QString>
#include <QDateTime>
#include <QMap>
#include "airinclient.h"

enum LogLevel { // this is for internal logging
    LL_NONE,
    LL_ERROR,
    LL_WARNING,
    LL_INFO,
    LL_DEBUG
};

enum LogOrder { // this is for LOG requests
    LogAscend,
    LogDescend
};


struct AirinMessage {
    int id;
    bool visible;
    QString name;
    QString message;
    QString color;
    QString login;
    QDateTime timestamp;
};

enum AirinBanState {
    BAN_NONE,
    BAN_SHADOW,
    BAN_FULL
};

struct AirinBanEntry {
    QString externalId;
    QString comment;
    QString stateTag;
    QString stateDescription;
    AirinBanState state;
};

struct AirinLogRequest {
    AirinClient *client;
    uint amount;
    uint from;
    LogOrder order;
};

#endif // AIRINDATA_H
