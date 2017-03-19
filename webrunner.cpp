#include "webrunner.h"

#include <QProcess>
#include <QDebug>

WebRunner::WebRunner(QObject *parent)
    :QThread( parent )
{

}

void WebRunner::run()
{
    QProcess process;
    process.start("/usr/local/bin/p3tv_web_api");

    process.waitForFinished(-1); // will wait forever until finished
}
