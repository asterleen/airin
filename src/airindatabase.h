#ifndef AIRINDATABASE_H
#define AIRINDATABASE_H

/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

#include <QObject>
#include <QString>
#include <QList>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QTimer>
#include <QMap>

#include "airindata.h"
#include "airinlogger.h"




class AirinDatabase : public QObject
{
    Q_OBJECT
public:
    explicit AirinDatabase(QObject *parent = 0);

    ~AirinDatabase();
    static AirinDatabase *db;

    enum DatabaseType {
        DatabaseMysql,
        DatabasePostgresql
    };

    void setDatabaseType(DatabaseType dbt);
    bool start(QString host, QString database, QString username, QString password);

    void setPing(uint minutes);

    QMap<QString, QVariant> getServerConfig();
    bool saveConfigValue (QString key, QString value);

    AirinBanState isUserBanned (QString userLogin);
    bool isUserAdmin (QString userLogin);

    int addMessage(QString authorLogin, QString text, QString name = QString(), QString color = QString(), bool isVisible = true);
    QList<AirinMessage>* getMessages(int amount, int from = 0, QString userLogin = QString());
    uint lastMessage();
    QString getUserId(QString internalToken);
    QString getMiscInfo (QString userLogin);
    bool killAuthSession(QString internalToken);

    QString whois (int messageId);

    // Admin-level commands
    bool setMessageStatus(int id, bool isActive);
    AirinMessage messageInfo (int id);
    bool setUserBanned (QString login, AirinBanState state, QString comment = QString());
    QStringList userNames(QString login);
    QList<AirinBanEntry> getBans();


private:
    QSqlDatabase database;
    uint lastMessageId;
    QTimer *pingTimer;

    DatabaseType dbType;

    void log (QString message, LogLevel level = LL_DEBUG);

private slots:
    void pingSqlServer();

signals:

public slots:
};

#endif // AIRINDATABASE_H
