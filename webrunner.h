#ifndef WEBRUNNER_H
#define WEBRUNNER_H

#include <QThread>

class WebRunner : public QThread
{
public:
    WebRunner( QObject* parent = NULL );

    virtual void run();
};

#endif // WEBRUNNER_H
