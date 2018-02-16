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

enum LogLevel {
    LL_NONE,
    LL_ERROR,
    LL_WARNING,
    LL_INFO,
    LL_DEBUG
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

#endif // AIRINDATA_H
