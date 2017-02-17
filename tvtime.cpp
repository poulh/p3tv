#include "tvtime.h"
#include "ui_tvtime.h"

TVTime::TVTime(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TVTime)
{
    ui->setupUi(this);
}

TVTime::~TVTime()
{
    delete ui;
}
