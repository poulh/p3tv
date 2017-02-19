#ifndef TVTIME_H
#define TVTIME_H

#include <QMainWindow>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QJsonDocument>
#include "qjsontablemodel.h"

namespace Ui {
class TVTime;
}

class TVTime : public QMainWindow
{
    Q_OBJECT

public:
    explicit TVTime(QWidget *parent = 0);
    ~TVTime();

private slots:
    void on_searchButton_clicked();

    void on_addSeriesButton_clicked();

    void on_deleteSeriesButton_clicked();

    void on_seriesTableView_clicked(const QModelIndex &index);

    void on_searchResultsTableView_doubleClicked(const QModelIndex &index);

    void on_downloadMissingButton_clicked();

    void on_catalogDownloadsButton_clicked();

private:
    QJsonDocument run_json_command( QStringList command );

    void refresh_series();

    Ui::TVTime *ui;
    QJsonTableModel *searchResults;
    QJsonTableModel *series;
    //QStandardItemModel *episodes;
    QJsonTableModel *episodes;
};

#endif // TVTIME_H
