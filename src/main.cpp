/*
        This is Airin 4, an advanced WebSocket chat server
     Licensed under the new BSD 3-Clause license, see LICENSE
            Made by Asterleen ~ https://asterleen.com
*/

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QString>
#include <QStringList>
#include "airinserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList() << "c" << "config", "Configuration file", "config"));
    parser.process(a);

    AirinServer airin(parser.value("config"));

    Q_UNUSED(airin);
    return a.exec();
}
