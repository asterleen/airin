#include "airinclient.h"

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

AirinClient::AirinClient(QWebSocket *sock, bool useXffHeader, QObject *parent) : QObject(parent)
{
    authorized = false;
    readonly = false;
    ready = false;
    adminMode = false;
    shadowBanned = false;
    disconnectEmitted = false;
    chatColorResets = 0;
    colorResetsMax = 0;
    pingMissTolerance = 0;
    pingMisses = -1;

    pingTimer = NULL;

    setSocket(sock, useXffHeader);
}

AirinClient::~AirinClient()
{
    ready = false;
}

void AirinClient::setSocket(QWebSocket *sock, bool useXffHeader)
{
    protocolApiLevel = 1;

    socket = sock;

    if (useXffHeader && sock->request().hasRawHeader(QByteArray("X-Forwarded-For")))
        clientRemoteAddress = "::ffff:"+QString(sock->request().rawHeader(QByteArray("X-Forwarded-For")));
    else
        clientRemoteAddress = sock->peerAddress().toString();

    connect(socket, SIGNAL(textMessageReceived(QString)), this, SLOT(sockMessageReceived(QString)));
    connect(socket, SIGNAL(disconnected()), this, SLOT(sockDisconnected()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(sockError(QAbstractSocket::SocketError)));

    ready = true;
}

void AirinClient::setInitTimeout(uint timeout)
{
    if (timeout > 0)
    {
        initTimer = new QTimer();
        connect (initTimer, SIGNAL(timeout()), this, SLOT(initTimedOut()));

        initTimer->setSingleShot(true);
        initTimer->start(timeout);
    }
}

void AirinClient::setSalt(QString salt)
{
    hashSalt = salt;
    clientHash = QString(QCryptographicHash::hash(QString(salt+clientRemoteAddress+salt).toUtf8(),
                                                  QCryptographicHash::Md5).toHex());

    clientChatColor = QString(QCryptographicHash::hash(
                                  QString(clientHash+QDateTime::currentDateTime().toString("dd.MM.yyyy")).toUtf8(),
                                  QCryptographicHash::Md5).toHex()).mid(0, 6);
}

void AirinClient::setApplication(QString app)
{
    clientApplication = app;
}

void AirinClient::setAuthorized(bool auth)
{
    if (auth)
        readonly = false; // can't be authorized and readonly at the same time

    initTimer->stop(); // init passed
    authorized = auth;
}

void AirinClient::setReadonly(bool ro)
{
    if (ro)
        authorized = false; // can't be authorized and readonly at the same time

    initTimer->stop(); // init passed
    readonly = ro;
}

void AirinClient::setAdminMode(bool admin)
{
    adminMode = admin;
}

void AirinClient::setInternalToken(QString internalToken)
{
    clientInternalToken = internalToken;
}

void AirinClient::setExternalId(QString externalId)
{
    clientExternalId = externalId;
}

void AirinClient::setChatName(QString name)
{
    clientChatName = name;
}

void AirinClient::setShadowBanned(bool shBanned)
{
    shadowBanned = shBanned;
}

void AirinClient::setChatColor(QString color)
{
    clientChatColor = color;
}

void AirinClient::setColorResetsMax(uint max)
{
    colorResetsMax = max;
}

void AirinClient::setApiLevel(uint apiLevel)
{
    protocolApiLevel = apiLevel;
}

void AirinClient::setPingTimeout(uint time, uint missTolerance)
{
    if (pingTimer == NULL)
    {
        pingMissTolerance = missTolerance;

        pingTimer = new QTimer(this);
        connect (pingTimer, SIGNAL(timeout()), this, SLOT(pingTimedOut()));

        pingTimer->start(time);
    }
}

bool AirinClient::resetChatColor()
{
    if (chatColorResets < colorResetsMax)
    {
        setChatColor(QString(QCryptographicHash::hash(
                                 QString(clientHash+QDateTime::currentDateTime().toString("dd.MM.yyyy:HH:mm:zzz")).toUtf8(),
                                 QCryptographicHash::Md5).toHex()).mid(0, 6));
        chatColorResets++;
        return true;
    }
    else
        return false;
}

void AirinClient::sendMessage(QString message)
{
    if (ready && socket->isValid() && socket->state() == QAbstractSocket::ConnectedState)
        socket->sendTextMessage(message);
}

void AirinClient::resetPingMisses()
{
    pingMisses = 0;
}

void AirinClient::close()
{
    socket->close();
    this->deleteLater();
}

QString AirinClient::hash()
{
    return clientHash;
}

QString AirinClient::app()
{
    return clientApplication;
}

QString AirinClient::remoteAddress()
{
    return clientRemoteAddress;
}

QString AirinClient::internalToken()
{
    return clientInternalToken;
}

QString AirinClient::externalId()
{
    return clientExternalId;
}

QString AirinClient::chatName()
{
    return clientChatName;
}

QString AirinClient::chatColor()
{
    return clientChatColor;
}

bool AirinClient::isAuthorized()
{
    return authorized;
}

bool AirinClient::isAdmin()
{
    return adminMode;
}

bool AirinClient::isShadowBanned()
{
    return shadowBanned;
}

bool AirinClient::isReadonly()
{
    return readonly;
}

bool AirinClient::isReady()
{
    return ready;
}

uint AirinClient::apiLevel()
{
    return protocolApiLevel;
}

void AirinClient::sockMessageReceived(QString message)
{
    emit messageReceived(message);
}

void AirinClient::sockDisconnected()
{
    ready = false;

    if (!disconnectEmitted)
    {
        disconnectEmitted = true;
        emit disconnected();
    }
}

void AirinClient::sockError(QAbstractSocket::SocketError error)
{
    ready = false;

    Q_UNUSED(error);
    if (!disconnectEmitted)
    {
        disconnectEmitted = true;
        emit disconnected();
    }
}

void AirinClient::initTimedOut()
{
    emit initTimeout();
}

void AirinClient::pingTimedOut()
{
    pingMisses++;

    emit pingTimeout();

    if ((uint)pingMisses >= pingMissTolerance)
        emit pingMissed();
}


