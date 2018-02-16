#include "airincommands.h"
#include "airinserver.h"

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

AirinCommands::AirinCommands(QObject *parent) : QObject(parent)
{

}

bool AirinCommands::process(QString commandLine, AirinClient *client, AirinServer *server)
{
    QStringList commands = commandLine.split(' ', QString::SkipEmptyParts);
    if (commands.count() == 0)
    {
        log ("Bad user-command (there's no command at all, lol)");
        return false;
    }

    QString mainCmd = commands.at(0);

    /// USER-LEVEL COMMANDS START HERE


    if (mainCmd == "help")
    {
        if (commands.count() < 2)
        {
            sendClientResponse(client,
                        "Available commands are: info, key, su, status, logoff");

            if (client->isAdmin())
                sendClientResponse(client, "[!] Administrative commands are: desu, whois, whowas, clients, restart, ban, disconnect, e, message, config, log");

            sendClientResponse(client,
                        "You can use /help on special commands, e.g. /help anon");
            sendClientResponse(client,
                        "More information here: https://github.com/asterleen/airin");

            return true;
        }

        if (commands[1] == "info")
        {
            sendClientResponse(client,
                        "/info: shows information about your connection.");
            sendClientResponse(client,
                        "Usage: /info");

            return true;
        }

        if (commands[1] == "key")
        {
            sendClientResponse(client,
                        "/key: displays your auth key for using in 3rd party applications.");
            sendClientResponse(client,
                        "Usage: /key");

            return true;
        }

        if (commands[1] == "status")
        {
            sendClientResponse(client,
                        "/status: displays current server status (version and online)");
            sendClientResponse(client,
                        "Usage: /status");

            return true;
        }

        if (commands[1] == "logoff")
        {
            sendClientResponse(client,
                        "/logoff: destroys your session and closes your connection.");
            sendClientResponse(client,
                        "Usage: /logoff");

            return true;
        }

        if (commands[1] == "su")
        {
            sendClientResponse(client,
                        "/su: elevates your privileges to Super User. Requires your login to be in white list.");
            sendClientResponse(client,
                        "Usage: /su");

            return true;
        }

        if (client->isAdmin())
        {
            if (commands[1] == "desu")
            {
                sendClientResponse(client,
                            "/desu: drops administrative privileges.");
                sendClientResponse(client,
                            "Usage: /desu");

                return true;
            }

            if (commands[1] == "e")
            {
                sendClientResponse(client,
                            "/e: returns you an arbitrary message as a server command. Useful for development");
                sendClientResponse(client,
                            "Usage: /e <your_test_message>");

                return true;
            }

            if (commands[1] == "whois")
            {
                sendClientResponse(client,
                            "/whois: shows author of specified message_id");
                sendClientResponse(client,
                            "Usage: /whois <message_id>");

                return true;
            }

            if (commands[1] == "whowas")
            {
                sendClientResponse(client,
                            "/whowas: shows all nicknames of specified user_login");
                sendClientResponse(client,
                            "Usage: /whowas <user_login>");

                return true;
            }

            if (commands[1] == "clients")
            {
                sendClientResponse(client,
                            "/clients: lists all clients that currently online");
                sendClientResponse(client,
                            "Usage: /clients");

                return true;
            }

            if (commands[1] == "restart")
            {
                sendClientResponse(client,
                            "/restart: performs restart of client or server, as specified");
                sendClientResponse(client,
                            "Usage: /restart <client|server>");

                return true;
            }

            if (commands[1] == "ban")
            {
                sendClientResponse(client,
                            "/ban: manage user access to the chat");
                sendClientResponse(client,
                            "Use /ban list to view existing blocks");
                sendClientResponse(client,
                            "Use /ban message to block special message's author");
                sendClientResponse(client,
                            "Use /ban login to block author by external ID (login)");
                sendClientResponse(client,
                            "Usage: /ban <(message|login)|list> [message_id|login] [none|shadow|full] [comment]");

                return true;
            }

            if (commands[1] == "disconnect")
            {
                sendClientResponse(client,
                            "/disconnect: drops all connections of specified criteria");
                sendClientResponse(client,
                            "Usage: /disconnect <login|hash|addr> <criteria>", UCR_WARNING);

                return true;
            }

            if (commands[1] == "config")
            {
                sendClientResponse(client,
                            "/config: configuration management. View and edit system configuration");
                sendClientResponse(client,
                            "Usage: /config <set|list> [<parameter> <value>]", UCR_WARNING);

                return true;
            }

            if (commands[1] == "log")
            {
                sendClientResponse(client,
                            "/log: live logging directly into your chat window");
                sendClientResponse(client,
                            "Usage: /log level <none|error|info|warning|debug>", UCR_WARNING);

                return true;
            }

            if (commands[1] == "message")
            {
                sendClientResponse(client,
                            "/message: allows operations with specified message_id");
                sendClientResponse(client,
                            "Available operations are: remove, restore, text, info");
                sendClientResponse(client,
                            "Usage: /message <message_id> <operation>");

                return true;
            }
        }

        sendClientResponse(client,
                    "No such command. Use /help <command> without slashes and brackets, e.g. /help anon");
        return true;
    }

    if (mainCmd == "info")
    {
        sendClientResponse(client,
                    QString("Your name is %1, UID is %2, color code is %3, application is %4.")
                    .arg(client->chatName())
                    .arg(client->hash())
                    .arg(client->chatColor())
                    .arg(client->app()));
        return true;
    }

    if (mainCmd == "key")
    {
        sendClientResponse(client,
                    QString("Your internal auth key is %1. Use it in 3rd party apps that don't support web authentication.").arg(client->internalToken()));

        return true;
    }

    if (mainCmd == "status")
    {
        uint dup;
        getClientStats(server, &dup);

        sendClientResponse(client,
                    QString("Airin/%1 Server OK, serving %2 active clients, %3 are duplicates.")
                    .arg(AIRIN_VERSION)
                    .arg(server->clientsCount())
                    .arg(dup));
        return true;
    }

    if (mainCmd == "logoff")
    {
        sendClientResponse(client, "You will be disconnected from the server.", UCR_WARNING);
        AirinDatabase::db->killAuthSession(client->internalToken());
        server->disconnectClientsByXId(client->externalId());
        return true;
    }

    /// ADMNINISTRATIVE COMMANDS START HERE

    if (mainCmd == "su")
    {
        if (AirinDatabase::db->isUserAdmin(client->externalId()))
        {
            client->setAdminMode(true);
            sendClientResponse(client, "Congratulations, you are in administrative mode! Be careful :3");
        }
        else
        {
            sendClientResponse(client, "You aren't allowed to use this command, m8.", UCR_ERROR);
        }

        return true;
    }

    if (client->isAdmin())
    {

        if (mainCmd == "desu")
        {
            client->setAdminMode(false);
            sendClientResponse(client, "Administrative mode disabled, desudesudesu!");
            AirinLogger::instance->setAdminClient(NULL);
            return true;
        }

        if (mainCmd == "whois")
        {
            if (commands.count() < 2)
            {
                sendClientResponse(client, "Usage: /whois <message id>", UCR_WARNING);
                return true;
            }

            int messageId = parseMessageId(commands[1]);
            if (messageId < 0)
            {
                sendClientResponse(client, "Message ID must be an integer.", UCR_WARNING);
                return true;
            }

            QString login = AirinDatabase::db->whois(messageId);
            if (login.isEmpty())
            {
                sendClientResponse(client, "No such message!", UCR_WARNING);
                return true;
            }

            sendClientResponse(client,
                               QString("Message was sent by client %1").arg(login));

            return true;
        }

        if (mainCmd == "whowas")
        {
            if (commands.count() < 2)
            {
                sendClientResponse(client, "Usage: /whowas <user login>", UCR_WARNING);
                return true;
            }

            QString login = commands[1];

            QStringList names = AirinDatabase::db->userNames(login);
            if (names.count() <= 0)
                sendClientResponse(client, "No usernames for this login or login is incorrect.", UCR_WARNING);
            else
                sendClientResponse(client, QString("Usernames of %1: %2")
                                   .arg(login).arg(names.join(", ")));
            return true;
        }

        if (mainCmd == "clients")
        {
            uint dup;
            QStringList clients = getClientStats(server, &dup);

            sendClientResponse(client,
                               QString("Listing %1 clients (%2 of them are duplicates)")
                               .arg(server->clientsCount())
                               .arg(dup));

            for (int i = 0; i < clients.length(); i++)
                sendClientResponse(client, clients.at(i));


            return true;
        }


        if (mainCmd == "restart")
        {
            if (commands.count() < 2)
            {
                sendClientResponse(client, "Usage: /restart <client|server>", UCR_WARNING);
                return true;
            }

            if (commands[1] == "client")
            {
                // This needs API level 3.
                server->messageBroadcast("RESTART #Please reconnect.", 3);
                sendClientResponse(client, "All clients with API Level > 3 are asked to restart.");
            }
                else
            if (commands[1] == "server")
            {
                serviceBroadcast(server, "Server is performing restart for maintenance.", UCR_WARNING);
                QTimer::singleShot(1000, server, SLOT(serverRestart()));
            }
            else
                sendClientResponse(client, "Usage: /restart <client|server>", UCR_WARNING);


            return true;
        }

        if (mainCmd == "ban")
        {
            if (commands.count() < 2)
            {
                sendClientResponse(client, "Usage: /ban <(message|login)|list> [message_id|login] [none|shadow|full] [comment]", UCR_WARNING);
                return true;
            }

            if (commands[1] == "list")
            {
                QList<AirinBanEntry> bans = AirinDatabase::db->getBans();

                if (bans.count() == 0)
                {
                    sendClientResponse(client, "No ban entries found.");
                    return true;
                }
                else
                {
                    sendClientResponse(client, "Listing banned logins");
                    for (int i = 0; i < bans.count(); i++)
                    {
                        AirinBanEntry ban = bans.at(i);
                        sendClientResponse(client,
                                           QString("Login %1; Type %2; Comment: %3")
                                           .arg(ban.externalId)
                                           .arg(ban.stateTag)
                                           .arg(ban.comment));
                    }

                    return true;
                }
            }

            if (commands[1] == "message" || commands[1] == "login")
            {
                if (commands.count() < 4)
                {
                    sendClientResponse(client, "Usage: /ban <message|login> <message_id|login> <none|shadow|full> [comment]", UCR_WARNING);
                    return true;
                }

                QString login;

                /// First, determine and check login to ban
                if (commands[1] == "message")
                {
                    int messageId = parseMessageId(commands[2]);
                    if (messageId <= 0)
                    {
                        sendClientResponse(client, "Message ID must be a positive integer.",
                                           UCR_WARNING);
                        return true;
                    }

                    AirinMessage message = AirinDatabase::db->messageInfo(messageId);
                    if (message.id != messageId)
                    {
                        sendClientResponse(client, "No such message!", UCR_WARNING);
                        return true;
                    }

                    login = message.login;
                }
                else
                    login = commands[2];


                /// It is impossible to shoot yourself in the foot
                if (login == client->externalId())
                {
                    sendClientResponse(client, "O rly? Are you going to ban yourself? No way!",
                                       UCR_ERROR);
                    return true;
                }


                /// Second, determine and check desired ban state
                AirinBanState banState;

                if (commands[3] == "none")
                    banState = BAN_NONE;
                else if (commands[3] == "shadow")
                    banState = BAN_SHADOW;
                else if (commands[3] == "full")
                    banState = BAN_FULL;
                else
                {
                    sendClientResponse(client, "Action must be either none, shadow or full",
                                       UCR_WARNING);
                    return true;
                }

                /// Try to extract comment
                QString comment = commandLine.mid(commandLine.indexOf(commands[3])+commands[3].length()+1).trimmed();

                /// If all the checks are passed, do requested operation
                if (AirinDatabase::db->setUserBanned(login, banState, comment))
                {
                    sendClientResponse(client,
                                       QString("Successfully changed ban state to %1 for user %2")
                                       .arg(commands[3]).arg(login));

                    server->setShadowbanned(login, (banState == BAN_SHADOW));

                    if (banState == BAN_FULL)
                    {
                        serviceBroadcast(server, "You've been banned. Sorry m8.",
                                         UCR_ERROR, login);
                        server->disconnectClientsByXId(login);
                    }
                }
                else
                {
                    sendClientResponse(client, "Could not ban user "+login, UCR_WARNING);
                }

                return true;
            } // end msg/login action

             sendClientResponse(client, "Usage: /ban <(message|login)|list> [message_id|login] [none|shadow|full] [comment]", UCR_WARNING);

        } // end ban command


        if (mainCmd == "disconnect")
        {
            if (commands.count() < 3)
            {
                sendClientResponse(client, "Usage: /disconnect <login|hash|addr> <parameter>", UCR_WARNING);
                return true;
            }

            if (commands[1] == "login")
            {
                server->disconnectClientsByXId(commands[2]);
                sendClientResponse(client, "Disconnected all clients by login");
            }
                else
            if (commands[1] == "hash")
            {
                server->disconnectClientsByXId(commands[2]);
                sendClientResponse(client, "Disconnected all clients by hash");
            }
                else
            if (commands[1] == "addr")
            {
                server->disconnectClientsByAddress(commands[2]);
                sendClientResponse(client, "Disconnected all clients by IP address");
            }
            else
                sendClientResponse(client, "Usage: /disconnect <login|hash|addr> <criteria>", UCR_WARNING);

            return true;
        }

        if (mainCmd == "e")
        {
            if (commands.count() < 2)
            {
                sendClientResponse(client, "Usage: /e <arbitrary message>", UCR_WARNING);
                return true;
            }

            client->sendMessage(commandLine.mid(commandLine.indexOf(' ')+1));
            return true;
        }

        if (mainCmd == "message")
        {
            if (commands.count() < 3)
            {
                sendClientResponse(client, "Usage: /message <msgid> <info|text|remove|restore>", UCR_WARNING);
                return true;
            }

            int messageId = parseMessageId(commands[1]);
            if (messageId <= 0)
            {
                sendClientResponse(client, "Message ID must be a positive integer.", UCR_WARNING);
                return true;
            }

            AirinMessage message = AirinDatabase::db->messageInfo(messageId);
            if (message.id != messageId)
            {
                sendClientResponse(client, "No such message!", UCR_WARNING);
                return true;
            }

            if (commands[2] == "info")
            {
                sendClientResponse(client,
                    QString("Info for message %1: author name is %2, login is %3, color code is %4, "
                            "UNIX timestamp is %5 (%6). This message is %7.")
                                   .arg(message.id)     // 1
                                   .arg(message.name)   // 2
                                   .arg(message.login)  // 3
                                   .arg(message.color)  // 4
                                   .arg(message.timestamp.toString("dd.MM.yyyy @ HH:mm:ss")) // 5
                                   .arg(message.timestamp.toTime_t())  // 6
                                   .arg((message.visible) ? "VISIBLE" : "NOT VISIBLE")); // 7

                return true;
            }

            if (commands[2] == "text")
            {
                sendClientResponse(client,
                    QString("Text of message %1 (unformatted): %2")
                                   .arg(message.id).arg(message.message));

                return true;
            }

            if (commands[2] == "remove")
            {
                if (AirinDatabase::db->setMessageStatus(messageId, false))
                {
                    // This method requires API level 3 and will be sent only to those who use it
                    server->messageBroadcast(QString("REMCON %1 #Remove me plz").arg(messageId), 3);

                    sendClientResponse(client, QString("Successfully removed message %1.").arg(messageId));
                }
                else
                    sendClientResponse(client, QString("Could not remove message %1!").arg(messageId));

                return true;
            }

            if (commands[2] == "restore")
            {
                if (AirinDatabase::db->setMessageStatus(messageId, true))
                    sendClientResponse(client, QString("Successfully restored message %1.").arg(messageId));
                else
                    sendClientResponse(client, QString("Could not restore message %1!").arg(messageId));

                return true;
            }

            sendClientResponse(client, "Usage: /message <msgid> <info|text|remove|restore>", UCR_WARNING);
            return true;
        }

        if (mainCmd == "config")
        {
            if (commands.count() < 2)
            {
                sendClientResponse(client, "Usage: /config <list|set>", UCR_WARNING);
                return true;
            }

            if (commands[1] == "set")
            {
                if (commands.count() < 4)
                {
                    sendClientResponse(client, "Usage: /config set <param> <value>", UCR_WARNING);
                    return true;
                }

                QString confValue = commandLine.mid(commandLine.indexOf(commands[2])+commands[2].length()+1).trimmed();

                if (AirinDatabase::db->saveConfigValue(commands[2], confValue))
                {
                    server->loadConfigFromDatabase();

                    sendClientResponse(client, "Settings have been updated and reloaded.");
                    sendClientResponse(client, "Keep in mind that some parameters may require a server restart to apply.");
                }
                else
                    sendClientResponse(client, "Could not update settings! Oh shish!", UCR_ERROR);
            }
                else
            if (commands[1] == "list")
            {
                sendClientResponse(client, "Listing server config parameters");
                QMap<QString, QVariant> config = AirinDatabase::db->getServerConfig();

                QList<QString> keys = config.uniqueKeys();

                for (int i = 0; i < keys.length(); i++)
                {
                    QString key = keys.at(i);
                    sendClientResponse(client, QString("%1=%2")
                                       .arg(key).arg(config.value(key).toString()));
                }

            }
            else
                sendClientResponse(client, "Usage: /config <list|set>", UCR_WARNING);


            return true;
        }

        if (mainCmd == "log")
        {
            if (commands.count() < 3)
            {
                sendClientResponse(client,
                            "Usage: /log level <none|error|info|warning|debug>", UCR_WARNING);
                return true;
            }

            if (commands[1] == "level")
            {
                LogLevel level;

                if (commands[2] == "debug")
                    level = LL_DEBUG;
                else if (commands[2] == "info")
                    level = LL_INFO;
                else if (commands[2] == "warning")
                    level = LL_WARNING;
                else if (commands[2] == "error")
                    level = LL_ERROR;
                else if (commands[2] == "none")
                    level = LL_NONE;
                else
                {
                    sendClientResponse(client, "Bad live logging mode set, doing nothing.", UCR_WARNING);
                    return true;
                }

                if (AirinLogger::instance->adminClient() == NULL ||
                    AirinLogger::instance->adminClient() != client)
                {
                    if (AirinLogger::instance->adminClient() != NULL)
                        sendClientResponse(AirinLogger::instance->adminClient(),
                                           "Another admin takes the live logging to themself!", UCR_WARNING);

                    AirinLogger::instance->setAdminClient(client);
                }

                AirinLogger::instance->setAdminLogLevel(level);

                sendClientResponse(client,
                                   QString("Live logging level is set to %1").arg(commands[2]));

                return true;
            }

            sendClientResponse(client,
                        "Usage: /log level <none|error|info|warning|debug>", UCR_WARNING);

            return true;
        }

        // Prevents occasional admin commands from being showed in the chat
        sendClientResponse (client, "No such command. Be careful, I said! ;3");
        return true;
    }

    return false;
}

void AirinCommands::sendClientResponse(AirinClient *client, QString message,
                                      AirinCommands::UserCmdResponse responseType)
{
    QString mode, color;
    switch (responseType)
    {
        case UCR_INFO    : mode = "INFO";    color = "00FFFF"; break;
        case UCR_WARNING : mode = "WARNING"; color = "ffff00"; break;
        case UCR_ERROR   : mode = "ERROR";   color = "ff0000"; break;
        default : mode = "INFO"; color = "00FFFF";
    }

    if (client->apiLevel() >= 2)
        client->sendMessage(QString("SERVICE %1 #%2").arg(mode).arg(message));
    else
        client->sendMessage(QString ("CONTENT 0 1452281488 *AirinService %1 #%2").arg(color).arg(message));
}

void AirinCommands::serviceBroadcast(AirinServer *server, QString message,
                                     AirinCommands::UserCmdResponse responseType,
                                     QString login)
{
    QList<AirinClient *> clients = server->getClients();

    for (int i = 0; i < clients.count(); i++)
    {
        if (!login.isEmpty())
        {
            if (clients.at(i)->externalId() != login)
                continue;
        }

        sendClientResponse(clients.at(i), message, responseType);
    }
}

int AirinCommands::parseMessageId(QString id)
{
    id.remove('>');
    bool convOk;

    int messageId = id.toInt(&convOk);
    return (convOk) ? messageId : -1;
}

QStringList AirinCommands::getClientStats(AirinServer *server, uint *duplicates)
{
    QStringList scannedIds, toReturn;
    *duplicates = 0;


    QList<AirinClient *>clients = server->getClients();

    for (int i = 0; i < clients.count(); i++)
    {
        if (clients.at(i)->isReadonly())
        {
            toReturn.append(QString("%3; UID %1 [READONLY]; App %2")
                            .arg(clients.at(i)->hash())
                            .arg(clients.at(i)->app())
                            .arg(clients.at(i)->remoteAddress()));
            continue;
        }

        if (clients.at(i)->externalId().isEmpty())
        {
            toReturn.append(clients.at(i)->remoteAddress() + " [INCOMPLETE]");
            continue;
        }

        QString clientData = QString("%1 (%2); UID %4; App %3")
                .arg(clients.at(i)->chatName())
                .arg(clients.at(i)->externalId())
                .arg(clients.at(i)->app())
                .arg(clients.at(i)->hash());

        if (scannedIds.indexOf(clients.at(i)->externalId()) > -1)
        {
            (*duplicates)++;
            clientData.append(" [DUPLICATE]");
        }
            else
        {
            scannedIds.append(clients.at(i)->externalId());
        }

        toReturn.append(clientData);
    }

    return toReturn;
}

void AirinCommands::log(QString message, LogLevel level)
{
    // just a shorthand
    AirinLogger::instance->log(message, level, "commd");
}
