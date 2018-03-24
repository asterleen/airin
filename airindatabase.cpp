#include "airindatabase.h"

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

AirinDatabase *AirinDatabase::db = 0;

AirinDatabase::AirinDatabase(QObject *parent) : QObject(parent)
{
    setDatabaseType(DatabaseMysql);
}

AirinDatabase::~AirinDatabase()
{

}

void AirinDatabase::setDatabaseType(AirinDatabase::DatabaseType dbt)
{
    dbType = dbt;
}

bool AirinDatabase::start(QString host, QString databaseName, QString username, QString password)
{
    QString dbName, dbDriver;

    switch (dbType)
    {
        case DatabaseMysql :
            dbName = "MySQL";
            dbDriver = "QMYSQL";
            break;

        case DatabasePostgresql :
            dbName = "PostgreSQL";
            dbDriver = "QPSQL";
            break;

        default :
            log ("Bad database type specified!", LL_ERROR);
            return false;
    }

    log (QString("Setting up a %3 database connection on %1@%2...")
         .arg(databaseName).arg(host).arg(dbName), LL_DEBUG);

    database = QSqlDatabase::addDatabase(dbDriver);
    database.setHostName(host);
    database.setDatabaseName(databaseName);
    database.setUserName(username);
    database.setPassword(password);

    log ("Attempting to connect...", LL_DEBUG);

    bool ok = database.open();

    if (!ok)
    {
        log (QString("Could not connect to the database: %1").arg(database.lastError().text()), LL_ERROR);
    }
    else
    {
        log ("Successfully connected to the database! :3", LL_INFO);
        log ("Trying to set up last message ID for further usage...");
        QSqlQuery qsqLastId;
        qsqLastId.exec("SELECT message_id FROM messages ORDER BY message_id DESC LIMIT 1");
        qsqLastId.next();
        lastMessageId = qsqLastId.value("message_id").toInt();

        log ((lastMessageId == 0)
             ? "Last message ID is zero. There's no messages or something went wrong..."
             : QString("Last message id is %1. It will be increased automatically.").arg(lastMessageId));
    }

    return ok;
}

void AirinDatabase::setPing(uint minutes)
{
    if (minutes >= 1)
    {
        log (QString("Airin will touch SQL server every %1 minute(s) to prevent its going away.")
             .arg(minutes));
        pingTimer = new QTimer(this);
        connect (pingTimer, SIGNAL(timeout()), this, SLOT(pingSqlServer()));
        pingTimer->start(minutes*60000);

    }
    else
        log("WTF are you doing? It is not possible to set ping less than one minute!", LL_WARNING);
}

QMap<QString, QVariant> AirinDatabase::getServerConfig()
{
    log ("Getting configuration from the database...");
    QMap<QString, QVariant> config;

    QSqlQuery qsqGetConfig;
    if (!qsqGetConfig.exec("SELECT conf_name, conf_value FROM server_config"))
    {
        log ("WARNING! Could not get settings from the database! Using defaults!", LL_WARNING);
        log ("Database says: "+qsqGetConfig.lastError().text(), LL_WARNING);
        log ("Query: "+qsqGetConfig.lastQuery());
        return config;
    }

    while (qsqGetConfig.next())
    {
        log (QString("Adding parameter %1, value %2").arg(qsqGetConfig.value("conf_name").toString())
                                                     .arg(qsqGetConfig.value("conf_value").toString()));

        config.insert(qsqGetConfig.value("conf_name").toString(), qsqGetConfig.value("conf_value"));

    }

    return config;
}

bool AirinDatabase::saveConfigValue(QString key, QString value)
{
    log (QString("Saving configuration entry, name %1, value %2").arg(key).arg(value));

    QSqlQuery qsqConfigSave;
    qsqConfigSave.prepare("SELECT conf_name FROM server_config WHERE conf_name = ?");
    qsqConfigSave.addBindValue(key);
    if (!qsqConfigSave.exec())
    {
        log ("WARNING! Could not save config: "+qsqConfigSave.lastError().text(), LL_WARNING);
        log ("Could not execute this: "+qsqConfigSave.lastQuery(), LL_DEBUG);
        return false;
    }
        else
    {
        QSqlQuery qsqSaver;
        if (!qsqConfigSave.next())
        {
            log ("Database does not have this parameter, creating new one.");
            qsqSaver.prepare("INSERT INTO server_config (conf_name, conf_value) VALUES (?, ?)");

            qsqSaver.addBindValue(key);
            qsqSaver.addBindValue(value);
        }
        else
        {
            log ("This parameter is known, will just update the existing entry.");
            qsqSaver.prepare("UPDATE server_config SET conf_value = ? WHERE conf_name = ?");

            qsqSaver.addBindValue(value);
            qsqSaver.addBindValue(key);
        }

        if (!qsqSaver.exec())
        {
            log ("WARNING! Could not save config: "+qsqConfigSave.lastError().text(), LL_WARNING);
            log ("Could not execute this: "+qsqConfigSave.lastQuery(), LL_DEBUG);
            return false;
        }
        else
            return true;
    }
}

AirinBanState AirinDatabase::isUserBanned(QString userLogin)
{
    QSqlQuery qsqBanCheck;
    qsqBanCheck.prepare("SELECT * FROM bans WHERE ban_login = ?");
    qsqBanCheck.addBindValue(userLogin);
    if (!qsqBanCheck.exec())
    {
        log ("Could not execute this: "+qsqBanCheck.lastQuery(), LL_DEBUG);
        log ("WARNING! Could not check BAN for user: "+qsqBanCheck.lastError().text(), LL_WARNING);
        return BAN_FULL;
    }
        else
    {
        if (!qsqBanCheck.next())
            return BAN_NONE;
        else
        {
            switch (qsqBanCheck.value("ban_state").toInt())
            {
                case 0 :
                    return BAN_NONE;
                    break;
                case 1 :
                    return BAN_SHADOW;
                    break;
                case 2 :
                    return BAN_FULL;
                    break;

                default :
                    log ("WARNING: Bad database value! YAEBAL!", LL_WARNING);
                    return BAN_NONE;
                    break;
            }
        }
    }
}

bool AirinDatabase::isUserAdmin(QString userLogin)
{
    QSqlQuery qsqAdminCheck;
    qsqAdminCheck.prepare("SELECT COUNT(*) AS cnt FROM admin_users WHERE user_login = ?");
    qsqAdminCheck.addBindValue(userLogin);
    if (!qsqAdminCheck.exec())
    {
        log ("Could not execute this: "+qsqAdminCheck.lastQuery(), LL_DEBUG);
        log ("WARNING! Could not check ADMIN ACL for user: "+qsqAdminCheck.lastError().text(), LL_WARNING);
        return true;
    }
        else
    {
        qsqAdminCheck.next();
        return (qsqAdminCheck.value("cnt").toInt() != 0); // this may be incorrect, plz tell me the right way
    }
}


int AirinDatabase::addMessage(QString authorLogin, QString text, QString name, QString color, bool isVisible)
{
    QSqlQuery qsqAdd;
    qsqAdd.prepare("INSERT INTO messages (message_author_login, message_text, "
                   "message_author_name, message_name_color, message_visible) VALUES (?,?,?,?,?)");
    qsqAdd.addBindValue(authorLogin);
    qsqAdd.addBindValue(text);
    qsqAdd.addBindValue(name);
    qsqAdd.addBindValue(color);
    qsqAdd.addBindValue(isVisible);
    if (!qsqAdd.exec())
    {
        log ("Could not execute this: "+qsqAdd.lastQuery(), LL_DEBUG);
        log ("Message addition SQL error: "+qsqAdd.lastError().text(), LL_WARNING);
        return -1;
    }
    else
    {
        lastMessageId = qsqAdd.lastInsertId().toInt();
        return lastMessageId;
    }
}

QList<AirinMessage> *AirinDatabase::getMessages(int amount, int from, QString userLogin)
{
    from = (from <= 0) ? lastMessageId - amount + 1 : from;
    QSqlQuery qsqGetMsg;
    qsqGetMsg.prepare("SELECT message_id, message_author_name, message_name_color, message_author_login, "
                                  // [!] UNIX_TIMESTAMP needs a custom function in pgsql!
                      "message_text, UNIX_TIMESTAMP(message_timestamp) as timestamp "
                      "FROM messages "
                      "where message_id >= ? AND (message_visible = true OR message_author_login = ?) order by message_id asc LIMIT ?");

    qsqGetMsg.addBindValue(from);
    qsqGetMsg.addBindValue(userLogin);
    qsqGetMsg.addBindValue(amount);

    if (!qsqGetMsg.exec())
    {
        log ("Could not execute this: "+qsqGetMsg.lastQuery(), LL_DEBUG);
        log ("Message fetching SQL error: "+qsqGetMsg.lastError().text(), LL_WARNING);
        return NULL;
    }
    else
    {
        log (QString("Building message list for %1 messages starting from %2 ID")
             .arg(amount).arg(from));

        QList<AirinMessage> *messages = new QList<AirinMessage>();
        while (qsqGetMsg.next())
        {
            AirinMessage msg;
            msg.id = qsqGetMsg.value("message_id").toInt();
            msg.message = qsqGetMsg.value("message_text").toString();
            msg.name = qsqGetMsg.value("message_author_name").toString();
            msg.timestamp = QDateTime::fromTime_t(qsqGetMsg.value("timestamp").toInt());
            msg.color = qsqGetMsg.value("message_name_color").toString();
            msg.login = qsqGetMsg.value("message_author_login").toString();
            messages->append(msg);
        }

        return messages;
    }
}

uint AirinDatabase::lastMessage()
{
    return lastMessageId;
}

QString AirinDatabase::getUserId(QString internalToken)
{
    QSqlQuery qsqUidGet;
    qsqUidGet.prepare("SELECT user_id, active FROM auth WHERE internal_token = ?");
    qsqUidGet.addBindValue(internalToken);
    if (!qsqUidGet.exec())
    {
        log ("Could not execute this: "+qsqUidGet.lastQuery(), LL_DEBUG);
        log ("WARNING! Could not get user_id for user: "+qsqUidGet.lastError().text(), LL_WARNING);
        return QString();
    }
        else
    {
        if (!qsqUidGet.first())
            return QString();
        else
        {
            if (!qsqUidGet.value("user_id").toString().isEmpty() &&
                qsqUidGet.value("active").toBool())
                return qsqUidGet.value("user_id").toString();
            else
                return QString();
        }
    }
}

QString AirinDatabase::getMiscInfo(QString userLogin)
{
    QSqlQuery qsqGetMisc;
    qsqGetMisc.prepare("SELECT misc_info FROM auth WHERE user_id = ?");
    qsqGetMisc.addBindValue(userLogin);
    if (!qsqGetMisc.exec())
    {
        log ("Could not execute this: "+qsqGetMisc.lastQuery(), LL_DEBUG);
        log ("WARNING! Could not get misc_info for user: "+qsqGetMisc.lastError().text(), LL_WARNING);
        return QString();
    }
        else
    {
        if (!qsqGetMisc.first())
            return QString();
        else
        {
            return qsqGetMisc.value("misc_info").toString().trimmed();
        }
    }
}

bool AirinDatabase::killAuthSession(QString internalToken)
{
    QSqlQuery qsqKillSession;
    qsqKillSession.prepare("UPDATE auth SET active = false WHERE internal_token = ?");
    qsqKillSession.addBindValue(internalToken);
    if (!qsqKillSession.exec())
    {
        log ("Could not execute this: "+qsqKillSession.lastQuery(), LL_DEBUG);
        log ("User session kill SQL error: "+qsqKillSession.lastError().text(), LL_WARNING);
        return false;
    }
    else
        return true;
}

QString AirinDatabase::whois(int messageId)
{
    QSqlQuery qsqWhois;
    qsqWhois.prepare("SELECT message_author_login FROM messages WHERE message_id = ?");
    qsqWhois.addBindValue(messageId);

    if (!qsqWhois.exec())
        return QString();
    else
    {
        if (qsqWhois.next())
        {
           return qsqWhois.value("message_author_login").toString();
        }
        else
           return QString();
    }
}

bool AirinDatabase::setMessageStatus(int id, bool isActive)
{
    QSqlQuery qsqSetMsgStatus;
    qsqSetMsgStatus.prepare("UPDATE messages SET message_visible=? WHERE message_id=?");
    qsqSetMsgStatus.addBindValue(isActive);
    qsqSetMsgStatus.addBindValue(id);

    return qsqSetMsgStatus.exec();
}


AirinMessage AirinDatabase::messageInfo(int id)
{
    QSqlQuery qsqGetMsg;
    qsqGetMsg.prepare("SELECT message_id, message_visible, message_author_name, message_author_login, message_name_color, "
                      "message_text, UNIX_TIMESTAMP(message_timestamp) as timestamp "
                      "FROM messages "
                      "where message_id = ?");

    qsqGetMsg.addBindValue(id);

    if (!qsqGetMsg.exec())
    {
        log ("Could not execute this: "+qsqGetMsg.lastQuery(), LL_DEBUG);
        log ("Message fetching SQL error: "+qsqGetMsg.lastError().text(), LL_WARNING);
        return AirinMessage();
    }
    else
    {
        AirinMessage msg;

        if (qsqGetMsg.next())
        {

            msg.id = qsqGetMsg.value("message_id").toInt();
            msg.visible = qsqGetMsg.value("message_visible").toBool();
            msg.message = qsqGetMsg.value("message_text").toString();
            msg.name = qsqGetMsg.value("message_author_name").toString();
            msg.login = qsqGetMsg.value("message_author_login").toString();
            msg.timestamp = QDateTime::fromTime_t(qsqGetMsg.value("timestamp").toInt());
            msg.color = qsqGetMsg.value("message_name_color").toString();
            return msg;
        }
            else return AirinMessage();
    }
}

bool AirinDatabase::setUserBanned(QString login, AirinBanState state, QString comment)
{
    QString query;

    QSqlQuery qsqBanCheck;
    qsqBanCheck.prepare("SELECT * FROM bans WHERE ban_login = ?");
    qsqBanCheck.addBindValue(login);
    if (!qsqBanCheck.exec())
    {
        log ("Could not execute this: "+qsqBanCheck.lastQuery(), LL_DEBUG);
        log ("WARNING! Could not check BAN for user: "+qsqBanCheck.lastError().text(), LL_WARNING);
        return false;
    }
        else
    {
        if (qsqBanCheck.next())
            query = "UPDATE bans SET ban_state = ?, ban_comment = ? WHERE ban_login = ?";
        else
            query = "INSERT INTO bans (ban_state, ban_comment, ban_login) VALUES (?, ?, ?)";
    }

    if (comment.isEmpty() || comment.isNull())
        comment = "Modified by Airin Admin tools";

    QSqlQuery qsqBanUser;
    qsqBanUser.prepare(query);
    qsqBanUser.addBindValue(state);
    qsqBanUser.addBindValue(comment);
    qsqBanUser.addBindValue(login);

    return qsqBanUser.exec();
}

QStringList AirinDatabase::userNames(QString login)
{
    QSqlQuery qsqUserNames;
    qsqUserNames.prepare("SELECT DISTINCT message_author_name FROM messages WHERE message_author_login = ?");
    qsqUserNames.addBindValue(login);

    if (!qsqUserNames.exec())
        return QStringList();
    else
    {
        QStringList names;
        while (qsqUserNames.next())
        {
            names.append(qsqUserNames.value("message_author_name").toString());
        }

        return names;
    }
}

QList<AirinBanEntry> AirinDatabase::getBans()
{
    QList<AirinBanEntry> bans;

    QSqlQuery qsqGetBans;
    if (!qsqGetBans.exec("SELECT ban_login, ban_comment, ban_state_tag, ban_state_description, ban_state as ban_state_id FROM bans INNER JOIN ban_states ON bans.ban_state = ban_states.ban_state_id WHERE ban_state_id <> 0"))
    {
        log ("WARNING! Could not get bans: "+qsqGetBans.lastError().text(), LL_WARNING);
        log ("Could not execute this: "+qsqGetBans.lastQuery(), LL_DEBUG);
    }
    else
    {
        while (qsqGetBans.next())
        {
            AirinBanEntry ban;
            ban.state = (AirinBanState)qsqGetBans.value("ban_state_id").toInt();
            ban.externalId = qsqGetBans.value("ban_login").toString();
            ban.comment = qsqGetBans.value("ban_comment").toString();
            ban.stateDescription = qsqGetBans.value("ban_state_description").toString();
            ban.stateTag = qsqGetBans.value("ban_state_tag").toString();

            bans.append(ban);
        }
    }

    return bans;
}

void AirinDatabase::log(QString message, LogLevel level)
{
    AirinLogger::instance->log(message, level, "dbase");
}

void AirinDatabase::pingSqlServer()
{
    // MySQL server tends to close connection if Airin doesn't
    // touch it for ~8h by default. OK, if Airin is alone,
    // she will talk to MySQL server because of boredom...
    QSqlQuery qsqPing;
    if (qsqPing.exec("SELECT 1"))
    {
        log ("Successfully touched the SQL server.");
    }
    else
    {
        log ("Could not touch the SQL server! Driver says this: "+qsqPing.lastError().text(),
             LL_WARNING);
    }
}

