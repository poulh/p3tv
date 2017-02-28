#include "p3tv.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    P3TV w;
    w.show();

    return a.exec();
}
