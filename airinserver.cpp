#include "airinserver.h"

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

AirinServer::AirinServer(QString config, QObject *parent) : QObject(parent), config(config)
{
    serverReady = false;

    if (!QFile::exists(config))
    {
        if (config.isEmpty())
            config = "<none>";

        printf ("Configuration file %s does not exist. Airin will exit. Sorry m8.\n", config.toUtf8().data());
        exit(-1);
    }

    loadConfig(config);

    AirinLogger::instance = new AirinLogger(logFile, (LogLevel)outputLogLevel);

    log ("Welcome to Airin 4 Chat Daemon! :3", LL_INFO);
    log ("You're running Airin/"+QString(AIRIN_VERSION));

    log ("Trying to set up database...");
    AirinDatabase::db = new AirinDatabase();

    AirinDatabase::DatabaseType dbt;

    if (sqlDbType == "mysql")
        dbt = AirinDatabase::DatabaseMysql;
    else
    if (sqlDbType == "pgsql")
        dbt = AirinDatabase::DatabasePostgresql;
    else
    {
        log ("Bad database type specified, exiting!");
        exit(1);
    }

    AirinDatabase::db->setDatabaseType(dbt);

    if (!AirinDatabase::db->start(sqlHost, sqlDatabase, sqlUsername, sqlPassword))
    {
        if (continueWithoutDB)
            log ("Could not set up database, DB functions won't be available!", LL_WARNING);
        else
        {
            log ("Could not set up the database, Airin will exit!", LL_ERROR);
            exit(2);
        }
    }
    else
        log ("Database connection established.", LL_INFO);

    // Database is ok? Lets fetch other settings!
    loadConfigFromDatabase();

    if (sqlServerPing > 0)
    {
        log ("Setting up database touch subservice...");
        AirinDatabase::db->setPing(sqlServerPing);
    }
    else
        log ("Airin will not touch the SQL server. Keep in mind that SQL servers can go away!");


    log (QString("Server will listen on %1 port.").arg(serverPort), LL_INFO);
    log ("Creating server instance...");

    server = new QWebSocketServer(QString("Airin/%1").arg(AIRIN_VERSION),
                                    (serverSecure)
                                    ? QWebSocketServer::SecureMode
                                    : QWebSocketServer::NonSecureMode,
                 this);

    if (serverSecure)
    {
        log ("Server will try to run in secure (WSS) mode.", LL_INFO);
        setupSsl();
    }
    else
    {
        log ("Server will run in non-secure (WS) mode.", LL_INFO);
    }


    log ("Starting to listen");
    if (!server->listen(QHostAddress::Any, serverPort))
    {
        log ("Could not start the server! Exiting.", LL_ERROR);
        exit(1);
    }

    connect (server, SIGNAL(newConnection()), this, SLOT(serverNewConnection()));
    log (QString("Airin listens on port %1 and waits for clients :3").arg(serverPort), LL_INFO);

    if (useLogRequestQueue)
    {
        logRequestQueueTimer = new QTimer(this);
        connect (logRequestQueueTimer, SIGNAL(timeout()), this, SLOT(flushLogRequestQueue()));
        logRequestQueueTimer->start(logQueueFlushTimeout);
    }


    serverReady = true; // ok to process new connections

}

AirinServer::~AirinServer()
{

}

uint AirinServer::clientsCount()
{
    return clients.count();
}

QList<AirinClient *> AirinServer::getClients()
{
    return clients;
}


void AirinServer::disconnectClientsByXId(QString externalId)
{
    for (int i = 0; i < clients.count(); i++)
    {
        if (clients.at(i)->externalId() == externalId)
            clients.at(i)->close();
    }
}

void AirinServer::disconnectClientsByAddress(QString address)
{
    for (int i = 0; i < clients.count(); i++)
    {
        if (clients.at(i)->remoteAddress() == "::ffff:"+address) // IPv6 Workaround
            clients.at(i)->close();
    }
}

void AirinServer::disconnectClientsByHash(QString hash)
{
    for (int i = 0; i < clients.count(); i++)
    {
        if (clients.at(i)->hash() == hash)
            clients.at(i)->close();
    }
}

void AirinServer::broadcastForXId(QString externalId, QString message)
{
    log (QString("Message broadcast for XID %1: %2").arg(externalId).arg(message));

    for (int i = 0; i < clients.count(); i++)
    {
        if (clients.at(i)->externalId() == externalId)
            clients.at(i)->sendMessage(message);
    }
}

void AirinServer::setShadowbanned(QString externalId, bool isShBanned)
{
    log (QString("Setting shadowban flag for XID %1").arg(externalId));

    for (int i = 0; i < clients.count(); i++)
    {
        if (clients.at(i)->externalId() == externalId)
            clients.at(i)->setShadowBanned(isShBanned);
    }
}

QString AirinServer::defaultChatName()
{
    return defaultUserName;
}


bool AirinServer::isOnline(QString externalId)
{
    for (int i = 0; i < clients.count(); i++)
    {
        if (clients.at(i)->externalId() == externalId)
            return true;
    }

    return false;
}


void AirinServer::loadConfig(QString configName)
{
    settings = new QSettings(configName, QSettings::IniFormat);

    settings->beginGroup("server");
    outputLogLevel = (LogLevel)settings->value("log_level", LL_DEBUG).toInt();
    if (outputLogLevel > 4)
        outputLogLevel = LL_DEBUG;

    logFile = settings->value("log_file", "stdout").toString();
    serverPort = settings->value("port", 1337).toUInt();
    if (serverPort <= 0 || serverPort > 65535)
        serverPort = 1337;

    initTimeout = settings->value("init_timeout", 0).toUInt();
    if (initTimeout > INT_MAX)
        initTimeout = 0;

    hashSalt = settings->value("secure_salt", "_replace_me_plz").toString();
    serverSecure = settings->value("secure_mode", false).toBool();
    sslCertFile = settings->value("ssl_certificate", "").toString();
    sslIntermediateCertFile = settings->value("ssl_intermediate_cert", "").toString();
    sslKeyFile = settings->value("ssl_key", "").toString();

    // Without a database Airin will use her defaults and won't be able to save messages
    continueWithoutDB = settings->value("continue_on_db_fault", false).toBool();

    // The settings above are used in load-time.
    // For runtime settings we will use values
    // from the database. More than that,
    // we can dynamically change these
    // using admin commands (/config)
    settings->endGroup();

    settings->beginGroup("external_auth");
    useXAuth = settings->value("enable", true).toBool(); // set this to 0 to simplify chat working mode
    settings->endGroup();

    settings->beginGroup("database");
    sqlDbType = settings->value("dbms", "mysql").toString();
    sqlHost = settings->value("hostname", "localhost").toString();
    sqlDatabase = settings->value("database", "airin").toString();
    sqlUsername = settings->value("username", "airin").toString();
    sqlPassword = settings->value("password", "paswd").toString();
    sqlServerPing = settings->value("server_ping", 0).toUInt();
    if (sqlServerPing > 7200) // 7200 minutes == 5 days
        sqlServerPing = 0;
    settings->endGroup();
}

void AirinServer::loadConfigFromDatabase()
{

    log ("Loading settings from database...", LL_INFO);

    QMap<QString, QVariant> config = AirinDatabase::db->getServerConfig();

    maxMessageAmount = config.value("message_amount_max", 500).toUInt();
    if (maxMessageAmount <= 0 || maxMessageAmount > INT_MAX)
        maxMessageAmount = 500;

    defaultMessageAmount = config.value("message_amount_default", 20).toUInt();
    if (defaultMessageAmount <= 0 || defaultMessageAmount > INT_MAX)
        defaultMessageAmount = 20;

    maxMessageLen = config.value("message_length", 2048).toUInt();
    if (maxMessageLen <= 0 || maxMessageLen > INT_MAX)
        maxMessageLen = 2048;

    maxNameLen = config.value("max_name_length", 20).toUInt();
    if (maxNameLen <= 0 || maxNameLen > 32) // SQL table allows 32 symbols max
        maxNameLen = 20;

    clientPingPollInterval = config.value("ping_poll_time", 10000).toUInt();
    if (clientPingPollInterval > 120000)
        clientPingPollInterval = 10000;

    clientPingMissTolerance = config.value("ping_miss_tolerance", 5).toUInt();
    if (clientPingMissTolerance > 50)
        clientPingMissTolerance = 5;

    colorResetMax = config.value("color_reset_max", 0).toUInt();
    if (colorResetMax > 32)
    {
        log ("Bad color reset max value! Setting to disabled state.", LL_WARNING);
        colorResetMax = 0;
    }

    minMessageDelay = config.value("message_delay", 5).toUInt(); // in seconds
    if (minMessageDelay <= 0 || minMessageDelay > 32) // delay of 32 seconds between messages is very slow for any chat
        minMessageDelay = 5;

    maxLogQueryQueueLength = config.value("max_log_queue_length", 50).toUInt();
    if (maxLogQueryQueueLength > 256)
        maxLogQueryQueueLength = 10;

    logQueueFlushTimeout = config.value("log_queue_flush_timeout", 500).toUInt();
    if (maxLogQueryQueueLength > 10000)
        maxLogQueryQueueLength = 500;

    delayTroll = config.value("delay_troll", false).toBool();
    defaultUserName = config.value("default_username", "Anonyamous").toString();
    readonlyAllowed = config.value("allow_readonly", true).toBool();
    checkNamesDistinctness = config.value("check_name_distinctness", false).toBool();
    forceDefaultName = config.value("force_default_name", false).toBool();
    discloseUserIds = config.value("disclose_user_ids", false).toBool();
    useMiscInfoAsName = config.value("use_misc_as_name", false).toBool();
    useLogRequestQueue = config.value("use_log_request_queue", false).toBool();
    useXffHeader = config.value("use_xff_header", false).toBool();
    deprecationMessage = config.value("deprecation_message", "Your API Level is deprecated, use higher one!").toString();

    log ("Database settings are loaded! :3", LL_INFO);
}


void AirinServer::setupSsl()
{
    QFile certFile(sslCertFile);
    QFile keyFile(sslKeyFile);
    if (!certFile.open(QIODevice::ReadOnly))
    {
        log ("Could not load SSL certificate from "+sslCertFile, LL_ERROR);
        exit(2);
    }

    if (!keyFile.open(QIODevice::ReadOnly))
    {
        log ("Could not load SSL key from "+sslKeyFile, LL_ERROR);
        exit(2);
    }

    QSslCertificate certificate(&certFile, QSsl::Pem);
    QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
    certFile.close();
    keyFile.close();

    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);

    if (sslIntermediateCertFile.isEmpty())
    {
        log ("Loading single SSL certificate as no intermediate cert file configured.");
        sslConfiguration.setLocalCertificate(certificate);
    }
    else
    {
        log ("Loading a chain of SSL certificates because intermediate certificate is also configured.");

        QList<QSslCertificate> chain;
        chain.append(certificate);

        QFile intermediateCertFile(sslIntermediateCertFile);
        if (!intermediateCertFile.open(QIODevice::ReadOnly))
        {
            log ("Could not load SSL intermediate certificate from "+sslIntermediateCertFile, LL_ERROR);
            // exit(2);
        }

        QSslCertificate intermediateCertificate(&intermediateCertFile, QSsl::Pem);
        intermediateCertFile.close();

        chain.append(intermediateCertificate);
        sslConfiguration.setLocalCertificateChain(chain);
    }
    sslConfiguration.setPrivateKey(sslKey);
    sslConfiguration.setProtocol(QSsl::TlsV1_0OrLater);

    server->setSslConfiguration(sslConfiguration);
}

void AirinServer::processClientCommand(AirinClient *client, QString command)
{
    QStringList commands = command.split(' ', QString::SkipEmptyParts);
    if (commands.count() == 0)
        return;

    QString mainCmd = QString(commands[0]).trimmed();
    if (mainCmd.isEmpty())
    {
        client->sendMessage("FAIL 299 #Lol wut");
        return;
    }

    /// LEVEL 1 ~ GENERIC AIRIN API

    if (mainCmd == "CONNECT" && (!client->isAuthorized() || client->isReadonly()))
    {
        if (commands.count() < 3 || !chkString(command))
        {
            log ("Bad client/auth information. Disconnecting", LL_WARNING);
            client->sendMessage("FAIL 299 #Syntax error");
            client->close();

            logAdmin(QString("Somebody (%1 / %2) mismatches the protocol!").arg(client->remoteAddress()).arg(client->hash()), LL_WARNING);
            return;
        }

        QString app = command.mid(command.indexOf("#")+1);

        if (app.length() > 256)
        {
            log ("Client tries to set too long app name, reducing");
            app = app.mid(0, 253)+"...";
        }

        client->setApplication(app);

        log(QString ("Client %1:%2 uses %3").arg(clients.indexOf(client))
            .arg(client->hash()).arg(client->app()));


            if (commands[1] == "READONLY")
            {
                if (!readonlyAllowed)
                {
                    client->sendMessage("FAIL 299 #Read-only mode is not allowed in server configuration.");
                    return;
                }

                if (client->apiLevel() < 2) // read-only is implemented in level 2
                {
                    client->sendMessage("FAIL 299 #Read-only mode requires API level 2");
                    logAdmin(QString("Somebody (%1 / %2) tries to use too high-leveled command").arg(client->remoteAddress()).arg(client->hash()));
                    return;
                }

                log ("Client decided to be in read-only mode. It will only be able to read chat.");
                client->setReadonly(true);
                client->sendMessage("AUTH READONLY #You are in Read-Only mode");
                return;
            }

        if (useXAuth)
        {
            client->setInternalToken(commands[1]);
            QString cachedUserId = AirinDatabase::db->getUserId(client->internalToken());

            if (!cachedUserId.isEmpty() && cachedUserId != "0")
            {
                log (QString("Client [%1:%2] passed auth process. Checking for banned status...")
                     .arg(clients.indexOf(client)).arg(client->hash()));

                client->setExternalId(cachedUserId);

                // This may conflict with the regex that checks usernames.
                // But we'll assume that web-frontend that usually
                // uses the SNS data will give us correct
                // names. Users still won't be able to write mess there.
                if (useMiscInfoAsName)
                {
                    QString miUserName = AirinDatabase::db->getMiscInfo(cachedUserId);
                    if (!miUserName.isEmpty())
                    {
                        miUserName = miUserName.replace(QRegExp("\\s"), ""); // spaces are special for us
                        client->setChatName(miUserName);
                    }
                }

                AirinBanState isBanned = AirinDatabase::db->isUserBanned(client->externalId());

                switch (isBanned)
                {
                    case BAN_NONE :
                        log ("Client isn't banned, accepting his authentication.");
                        client->sendMessage("AUTH OK #You are welcome! :3");
                        client->setAuthorized(true);

                        logAdmin(QString("A new client connected and authorized, login %1, app %2")
                                        .arg(client->externalId()).arg(client->app()),
                                 LL_INFO);
                        break;

                    case BAN_SHADOW :
                        log ("Pssst, client is shadowbanned. We'll be maximally quiet! :3");
                        client->setShadowBanned(true);
                        client->sendMessage("AUTH OK #You are welcome.");
                        client->setAuthorized(true);

                        logAdmin(QString("A new client connected and shadowbanned, login %1, app %2")
                                        .arg(client->externalId()).arg(client->app()),
                                 LL_INFO);

                        break;

                    case BAN_FULL :
                        log ("Client is banned, declining him.");
                        logAdmin(QString("A banned client tried to connect, login %1, app %2")
                                        .arg(client->externalId()).arg(client->app()),
                                 LL_INFO);

                        client->sendMessage("AUTH BANNED #Sorry but your account is not allowed to be used with this chat.");
                        client->close();

                        break;
                }

                if (client->apiLevel() < AIRIN_MIN_API_LEVEL)
                    AirinCommands::sendClientResponse(client, deprecationMessage, AirinCommands::UCR_WARNING);
            }
            else
            {
                log (QString("Client [%1:%2] has no internal token, not authenticated!")
                     .arg(clients.indexOf(client)).arg(client->hash()));
                client->sendMessage("AUTH FAIL #Your auth key is invalid ._.");
            }
        }
        else
        {
            log ("Authenticated client unconditionally as xauth module is not enabled.", LL_INFO);
            client->setAuthorized(true);
            client->sendMessage("AUTH OK #Airin does not require auth :3");
        }
        return;
     }

    if (mainCmd == "LEVEL")
    {
        bool levelOk;
        uint level = commands.at(1).toUInt(&levelOk);

        if (!levelOk || level <= 0 || level > AIRIN_MAX_API_LEVEL)
        {
            client->sendMessage(QString("FAIL 299 #Bad API level is set, current level is %1").arg(AIRIN_MAX_API_LEVEL));
            return;
        }
        else
        {
            if (level > client->apiLevel())
            {
                client->setApiLevel(level);
                client->sendMessage(QString("LEVEL %1 OK #Level-Up! :3").arg(level));

                if (level >= 3) // initialize additional client capabilities at level 3
                {
                    if (clientPingMissTolerance > 0 && clientPingPollInterval > 0)
                    {
                            log (QString ("Setting ping interval (%1 ms) and miss tolerance (%2 ms)...")
                                 .arg(clientPingPollInterval)
                                 .arg(clientPingMissTolerance));

                            client->setPingTimeout(clientPingPollInterval, clientPingMissTolerance);
                    }
                }
            }
        }
    }

    if (mainCmd == "CONTENT" && checkAuth(client))
    {
        if (commands.count() < 3 || !chkString(command))
        {
            log ("Client tries to send inappropriate data, declining.", LL_WARNING);
            logAdmin(QString("Somebody (%1 / %2) mismatches the protocol!").arg(client->remoteAddress()).arg(client->hash()), LL_WARNING);

            client->sendMessage("FAIL 299 #Syntax error");
            client->close();
            return;
        }

        processMessage(client, commands[1], command.mid(command.indexOf("#")+1));
        return;
    }

    if (mainCmd == "IAM" && checkAuth(client) && !forceDefaultName)
    {
        if (commands.count() < 2 || !chkString(command))
        {
            log ("Client tries to set strange name, declining.", LL_WARNING);
            client->sendMessage("FAIL 299 #Syntax error");
            client->close();
            return;
        }

        QString clName = command.mid(command.indexOf("#")+1).trimmed();
        if (QRegExp(QString("^[a-z0-9а-яА-ЯЁё]{1,%1}$").arg(maxNameLen),
                    Qt::CaseInsensitive).exactMatch(clName))
        {
            if (checkNamesDistinctness && !isNameDistinct(client))
            {
                client->sendMessage("FAIL 207 #Name is not unique, please choose another.");
                return;
            }

            client->setChatName(clName);
            client->sendMessage("NTM #"+clName);

            logAdmin(QString("Client with login %1 sets name to %2")
                            .arg(client->externalId()).arg(clName));
        }
        else
        {
            client->setChatName(defaultUserName);
            if (!clName.isEmpty())
                client->sendMessage("FAIL 203 #Name should not contain anything excepting latin symbols and numbers. No spaces plz.");
            else
                client->sendMessage("NTM #");
        }

        return;
    }

    if (mainCmd == "LOG" && (client->isReadonly() || checkAuth(client)))
    {

        switch (commands.count())
        {

            case 1 :
                processMessageAPI(client, 0);
                break;

            case 2 :
                processMessageAPI(client, commands[1]);
                break;

            case 3 :
                if (commands[2] == "DESC")
                    processMessageAPI(client, commands[1], 0, LogDescend);
                    else
                if (commands[2] == "ASC")
                    processMessageAPI(client, commands[1], 0, LogAscend);
                else
                    processMessageAPI(client, commands[1], commands[2]);
                break;

            case 4 :
                if (commands[3] == "DESC")
                    processMessageAPI(client, commands[1], commands[2], LogDescend);
                    else
                if (commands[3] == "ASC")
                    processMessageAPI(client, commands[1], commands[2], LogAscend);
                else
                    processMessageAPI(client, commands[1], commands[2]);
                break;

            default :
                log ("Client tries to use message API wrong, declining.", LL_WARNING);
                logAdmin(QString("Somebody (%1 / %2) mismatches the protocol!")
                         .arg(client->remoteAddress()).arg(client->hash()), LL_WARNING);
                client->sendMessage("FAIL 299 #Syntax error");
                client->close();
                return;
                break;
        }

        return;
    }

    if ((mainCmd == "LOGOFF" || mainCmd == "DOWNGRADE") && checkAuth(client))
    {
        if (commands.count() < 2)
        {
            log ("Client tries to disconnect incorrectly, declining.", LL_WARNING);
            client->sendMessage("FAIL 299 #Syntax error");
            client->close();
            return;
        }

        log (QString("Client [%1:%2] changes its auth state.")
             .arg(clients.indexOf(client)).arg(client->hash()));

        if (mainCmd == "LOGOFF")
        {
            if (AirinDatabase::db->killAuthSession(commands[1]))
            {
                client->sendMessage(QString("LOGOFF OK #Bye ._.").arg(mainCmd));
            }
            else
            {
                client->sendMessage("LOGOFF FAIL #I can't kill this session");
            }

            client->close();
        }

        if (mainCmd == "DOWNGRADE")
        {
            if (client->apiLevel() < 2)
            {
                client->sendMessage("FAIL 299 #You should have API level 2 to use this.");
                return;
            }

            if (readonlyAllowed)
            {
                client->setReadonly(true);
                client->sendMessage(QString("DOWNGRADE OK #You're read-only now").arg(mainCmd));
            }
            else
            {
                client->sendMessage("FAIL 299 #Read-only mode is not allowed by server configuration");
            }
        }
        return;
    }

    /// LEVEL 2 : COMMANDS + REPORTS SUPPORT

    if (client->apiLevel() < 2)
    {
        client->sendMessage("FAIL 299 #Unknown command or not implemented in your API level (needs level 2)");
        return;
    }

    // There were some functions that are specific to provoda.ch
    // So this level is useless in open source version of Airin.
    // Switch to 3rd level instead.

    /// LEVEL 3 : MESSAGE REMOVAL, REMOTE PARAMETERS, RESTART, SUS

    if (client->apiLevel() < 3)
    {
        client->sendMessage("FAIL 299 #Unknown command or not implemented in your API level (needs level 3)");
        return;
    }

    if (mainCmd == "GETSET")
    {
        client->sendMessage(QString("SET nick_regex #^[a-z0-9а-яА-ЯЁё]{1,%1}$").arg(maxNameLen));
        client->sendMessage(QString("SET max_name_length #%1").arg(maxNameLen));
        client->sendMessage(QString("SET max_message_length #%1").arg(maxMessageLen));
        client->sendMessage(QString("SET min_message_delay #%1").arg(minMessageDelay));
        client->sendMessage(QString("SET max_log_message_amount #%1").arg(maxMessageAmount));
        client->sendMessage(QString("SET color_reset_attempts #%1").arg(colorResetMax));
        client->sendMessage(QString("SET logins_disclosed #%1").arg(discloseUserIds ? "1" : "0"));

        return;
    }

    if (mainCmd == "SUS")
    {
        client->resetPingMisses();
        return;
    }


}

void AirinServer::processMessage(AirinClient *client, QString recCode, QString message)
{
    uint lastTime = lastMessageTime.value(client->externalId(), 0),
         now = QDateTime::currentDateTime().toTime_t();

    log (QString("Client %1 sent a message. His last message time is %2, now is %3.")
         .arg(client->externalId()).arg(lastTime).arg(now));

    if (lastTime == 0 || now - lastTime > minMessageDelay)
    {
        lastMessageTime[client->externalId()] = now;
        message = message.trimmed();

        if (message.startsWith('/'))
        {
            QString cmd = message.mid(1);
            if (AirinCommands::process(cmd, client, this))
            {
                log (QString("Client [%1:%2] sent a command instead of a message, won't even save it.")
                     .arg(clients.indexOf(client)).arg(client->hash()));
                client->sendMessage("CONREC "+recCode+" 0");
                return;
            }
        }

        if (message.length() == 0 || (unsigned)message.length() > maxMessageLen)
        {
            client->sendMessage("FAIL 204 #Message is too long or doesn't exist at all");
            log ("Client sent something bad, haha loser!");
            logAdmin(QString("Client %1 (%2) sent a bad message, ignored.")
                            .arg(client->chatName()).arg(client->externalId()), LL_WARNING);
        }
        else
        {
            int messageId = AirinDatabase::db->addMessage(client->externalId(), message,
                                                          client->chatName(), client->chatColor(),
                                                          !client->isShadowBanned());
            if (messageId > -1)
            {
                log ("Message saved successfully with id "+QString::number(messageId));
                client->sendMessage(QString("CONREC %1 %2").arg(recCode).arg(messageId));
            }
                else
            {
                client->sendMessage("FAIL 299 #Internal Airin error");
                log ("Could not save message! Fcuk!", LL_WARNING);
                logAdmin(QString("WARNING! Database error, see system logs!"), LL_WARNING);
            }

            // Messages should be sent independently on database
            QString messageCommand = QString("CONTENT %1 %2 %3 %4 %5 #%6")
                    .arg((messageId > 0) ? messageId : 0)
                    .arg(QDateTime::currentDateTime().toTime_t())
                    .arg(client->chatName())
                    .arg(client->chatColor())
                    .arg(discloseUserIds ? client->externalId() : "null")
                    .arg(message);


            // Shadowban: message will be visible ONLY for client if it is shadowbanned
            if (client->isShadowBanned())
                broadcastForXId(client->externalId(), messageCommand);
            else
                messageBroadcast(messageCommand);
        }
    }
        else
    {
        log (QString("Client %1 tries to send messages too fast! GOLAKTEKO OPASNOSTE!")
             .arg(client->externalId()));

        logAdmin(QString("Client %1 (%2) floods the chat, oh shit!")
                        .arg(client->chatName()).arg(client->externalId()), LL_WARNING);

        client->sendMessage(QString("FAIL 205 %1 #Don't flood! Message frequency is limited to %1 seconds.")
                            .arg(minMessageDelay));
        if (delayTroll)
        {
            log ("Delay trolling is enabled! Users will be blocked until they'll wait the delay limit.");
            lastMessageTime[client->externalId()] = now;
        }

    }
}

void AirinServer::processMessageAPI(AirinClient *client, QString messageAmount,
                                    QString messageOffset, LogOrder order)
{
    bool valueCorrect;
    uint amount = messageAmount.toInt(&valueCorrect);

    if (!valueCorrect || amount <= 0 || amount > maxMessageAmount)
    {
        client->sendMessage("FAIL 299 #Bad amount parameter");
        return;
    }

    uint offset = messageOffset.toInt(&valueCorrect);

    if (!valueCorrect || offset <= 0 || offset >= AirinDatabase::db->lastMessage())
    {
        log (QString("Client requested no or wrong offset, will send last %1 messages.").arg(amount));
        offset = -1;
    }

    AirinLogRequest req;
    req.amount = amount;
    req.from = offset;
    req.order = order;
    req.client = client;


    if (useLogRequestQueue)
        enqueueLogRequest(req);
    else
        respondLogRequest(req);

}

void AirinServer::enqueueLogRequest(AirinLogRequest req)
{
    if (!req.client->isReady())
        return;

    if ((uint)logRequests.count() < maxLogQueryQueueLength)
        logRequests.append(req);
    else
        req.client->sendMessage("FAIL 299 #Log request queue is full, wait plz");
}

void AirinServer::respondLogRequest(AirinLogRequest req)
{
    // This is the best and fault-free way to check is our client is still valid.
    // If you know a better way to check the object existence and validity in C++/Qt
    // feel free to send us a pull request at github.com/asterleen/airin
    if (clients.indexOf(req.client) == -1)
    {
        log ("Trying to send logs to a disconnected client, aborting");
        return;
    }

    QList<AirinMessage> *messages;

    if (req.from > 0)
    {
        messages = AirinDatabase::db->getMessages(req.amount, req.from, req.client->externalId());
    }
        else // user requested just last N messages
    {
        messages = AirinDatabase::db->getMessages(req.amount, 0, req.client->externalId());
    }

    if (messages == NULL)
    {
        log ("Database returned bad messages list", LL_WARNING);
        return;
    }

    int msCnt = messages->count(); // just caching, nothing special
    if (msCnt > 0)
    {
        if (req.order == LogDescend)
        {
            for (int i = msCnt - 1; i >= 0; i--)
            {
                req.client->sendMessage(QString("LOGCON %1 %2 %3 %4 %5 #%6")
                                    .arg(messages->at(i).id)
                                    .arg(messages->at(i).timestamp.toTime_t())
                                    .arg((messages->at(i).name.isEmpty())
                                         ? defaultUserName : messages->at(i).name)
                                    .arg (messages->at(i).color.isEmpty()
                                         ? "NULL" : messages->at(i).color)
                                    .arg(discloseUserIds ? messages->at(i).login : "null")
                                    .arg(messages->at(i).message));
            }
        }
        else
        {
            for (int i = 0; i < msCnt; i++)
            {
                req.client->sendMessage(QString("LOGCON %1 %2 %3 %4 %5 #%6")
                                    .arg(messages->at(i).id)
                                    .arg(messages->at(i).timestamp.toTime_t())
                                    .arg((messages->at(i).name.isEmpty())
                                         ? defaultUserName : messages->at(i).name)
                                    .arg (messages->at(i).color.isEmpty()
                                         ? "NULL" : messages->at(i).color)
                                    .arg(discloseUserIds ? messages->at(i).login : "null")
                                    .arg(messages->at(i).message));
            }
        }
    }
    else
    {
        log ("There were no messages matching client's request");
        req.client->sendMessage("FAIL 206 #No messages");
    }

    delete messages;
}

void AirinServer::sendGreeting(AirinClient *client)
{
    client->sendMessage("REM #      /\\_/\\");
    client->sendMessage("REM # ____/ o o \\");
    client->sendMessage("REM #/~____  =w= /");
    client->sendMessage("REM #(______)__m_m)");
}

void AirinServer::messageBroadcast(QString message, uint apiLevel)
{
    log (QString("Broadcast message for %1 clients: %2").arg(clients.count()).arg(message));

    for (int i = 0; i < clients.count(); i++)
    {
        if (clients.at(i)->isAuthorized() || clients.at(i)->isReadonly())
        {
            if (apiLevel == 0 ||
               (apiLevel > 0 && clients.at(i)->apiLevel() >= apiLevel))
                    clients.at(i)->sendMessage(message);
        }
    }
}

bool AirinServer::chkString(const QString &s)
{
    return (s.indexOf('#') > -1);
}

bool AirinServer::checkAuth(AirinClient *client)
{
    if (client->isAuthorized())
        return true;
    else
    {
        client->sendMessage("FAIL 200 #You still aren't authorized. Use CONNECT command.");
        return false;
    }
}

bool AirinServer::isNameDistinct(AirinClient *client)
{
    QString name = client->chatName().toLower();

    for (int i = 0; i < clients.length(); i++)
    {
        AirinClient *tmpClient = clients.at(i); // cache

        if (client->externalId() != tmpClient->externalId() &&
            tmpClient->chatName() != defaultUserName &&
            name == tmpClient->chatName().toLower())
            return false;
    }

    return true;
}

void AirinServer::log(QString message, LogLevel logLevel, QString component)
{
    AirinLogger::instance->log(message, logLevel, component);
}

void AirinServer::logAdmin(QString message, LogLevel logLevel)
{
    AirinLogger::instance->logToAdmin(message, logLevel);
}


void AirinServer::serverNewConnection()
{
    while (server->hasPendingConnections())
    {
        log ("A new connection detected, processing client...");
        QWebSocket *sock = server->nextPendingConnection();

        if (!serverReady)
        {
            sock->sendTextMessage("FAIL 299 #Not started yet, plz wait!");
            sock->close();
            sock->deleteLater();
            continue;
        }

        AirinClient *client = new AirinClient(sock, useXffHeader);

        if (sock->request().hasRawHeader(QByteArray("X-Forwarded-For")))
        {
            if (useXffHeader)
                log ("It seems that I'm behind the reverse proxy, I'll read X-Forwarded-For header to obtain the IP address");
            else
                log ("An X-Forwarded-For header is received but the server is configured to ignore it", LL_INFO);
        }

        log (QString("Client address: %1").arg(client->remoteAddress()));
        log ("Enhashing client's address...");
        client->setSalt(hashSalt);
        log (QString("Client UID (hash): %1").arg(client->hash())); // this is made because of historical reasons. we're sorry.

        log ("Initializing client's capabilities...");
        connect (client, SIGNAL(messageReceived(QString)), this, SLOT(clientMessage(QString)));
        connect (client, SIGNAL(disconnected()), this, SLOT(clientDisconnect()));
        connect (client, SIGNAL(initTimeout()), this, SLOT(clientInitTimeout()));
        connect (client, SIGNAL(pingTimeout()), this, SLOT(clientPingTimeout()));
        connect (client, SIGNAL(pingMissed()), this, SLOT(clientPingMissed()));

        log ("Setting client's default values...");

        client->setColorResetsMax(colorResetMax);
        client->setChatName(defaultUserName);
        clients.append(client);
        log (QString("Client [%1:%2 / %3] initialized successfully, greeting him and starting INIT process.")
             .arg(clients.indexOf(client)).arg(client->hash()).arg(client->remoteAddress()), LL_INFO);

        log (QString ("Setting a timeout watchdog for %1 ms...").arg(initTimeout));
        client->setInitTimeout(initTimeout);

        sendGreeting(client);
        client->sendMessage(QString("INIT #AirinServer/%1 ~ All SAS Oelutz!").arg(AIRIN_VERSION));

    }

}

void AirinServer::clientMessage(QString message)
{
    AirinClient *client = (AirinClient *)QObject::sender();
    log (QString("Client [%3:%1] says '%2'").arg(client->hash()).arg(message).arg(clients.indexOf(client)));

    processClientCommand(client, message);
}

void AirinServer::clientDisconnect()
{
     AirinClient *client = (AirinClient *)QObject::sender();
     log (QString("Client [%3:%1 / %2] leaves us...").arg(client->hash()).arg(client->remoteAddress())
          .arg(clients.indexOf(client)), LL_INFO);

     clients.removeAt(clients.indexOf(client));
     client->deleteLater();
}

void AirinServer::clientInitTimeout()
{
    AirinClient *client = (AirinClient *)QObject::sender();

    log (QString("Client [%3:%1 / %2] did not pass neccessary init process, disconnecting him.").arg(client->hash()).arg(client->remoteAddress())
         .arg(clients.indexOf(client)), LL_WARNING);

    logAdmin(QString("Client %1 did not pass neccessary init process, disconnecting him!")
                    .arg(client->remoteAddress()), LL_WARNING);

    client->close();
}

void AirinServer::clientPingTimeout()
{
    AirinClient *client = (AirinClient *)QObject::sender();
    client->sendMessage("NUS");
}

void AirinServer::clientPingMissed()
{
    AirinClient *client = (AirinClient *)QObject::sender();

    logAdmin(QString("Client %1 (%2) disconnected because of ping timeout")
                    .arg(client->externalId()).arg(client->hash()));

    client->close();
}

void AirinServer::serverRestart()
{
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}

void AirinServer::flushLogRequestQueue()
{
    if (logRequests.count() > 0)
    {
        respondLogRequest(logRequests.first());
        logRequests.removeFirst();
    }
}
