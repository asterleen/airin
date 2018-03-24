#ifndef AIRINCLIENT_H
#define AIRINCLIENT_H

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

#include <QObject>
#include <QWebSocket>
#include <QString>
#include <QCryptographicHash>
#include <QDateTime>
#include <QTimer>

class AirinClient : public QObject
{
    Q_OBJECT
public:
    explicit AirinClient(QWebSocket *sock, bool useXffHeader = false, QObject *parent = 0);
    ~AirinClient();

    enum BanType
    {
        BanNone,
        BanFull,
        BanShadow
    };

    void setSocket (QWebSocket *sock, bool useXffHeader);
    void setInitTimeout (uint timeout);
    void setSalt(QString salt);
    void setApplication (QString app);
    void setAuthorized (bool auth);
    void setReadonly (bool ro);
    void setAdminMode (bool admin);
    void setInternalToken (QString internalToken);
    void setExternalId(QString externalId);
    void setChatName(QString name);
    void setShadowBanned(bool shBanned);
    void setChatColor(QString color);
    void setColorResetsMax(uint max);
    void setApiLevel(uint apiLevel);
    void setPingTimeout (uint time, uint missTolerance);

    bool resetChatColor();

    void sendMessage(QString message);
    void resetPingMisses();
    void close();

    QString hash();
    QString app();
    QString remoteAddress();
    QString internalToken();
    QString externalId();
    QString chatName();
    QString chatColor();
    bool isAuthorized();
    bool isAdmin();
    bool isShadowBanned();
    bool isReadonly();
    bool isReady();
    uint apiLevel();


private:
    QWebSocket *socket;

    QTimer *initTimer;
    QTimer *pingTimer;

    QString clientToken;
    QString clientExternalId;
    QString clientApplication;
    QString clientInternalToken;
    QString clientHash;
    QString clientChatName;
    QString clientChatColor;
    QString clientRemoteAddress;
    QString hashSalt;


    bool authorized;
    bool readonly;
    bool ready;
    bool adminMode;
    bool shadowBanned;
    int pingMisses;
    uint chatColorResets;
    uint colorResetsMax;
    uint pingMissTolerance;

    bool disconnectEmitted;

    uint protocolApiLevel;

private slots:
    void sockMessageReceived (QString message);
    void sockDisconnected();
    void sockError (QAbstractSocket::SocketError error);

    void initTimedOut();
    void pingTimedOut();


signals:
    void messageReceived(QString);
    void initTimeout();
    void pingTimeout();
    void pingMissed();
    void disconnected();


};

#endif // AIRINCLIENT_H
