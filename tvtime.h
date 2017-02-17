#ifndef TVTIME_H
#define TVTIME_H

#include <QMainWindow>

namespace Ui {
class TVTime;
}

class TVTime : public QMainWindow
{
    Q_OBJECT

public:
    explicit TVTime(QWidget *parent = 0);
    ~TVTime();

private:
    Ui::TVTime *ui;
};

#endif // TVTIME_H
