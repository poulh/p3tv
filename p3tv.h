#ifndef TVTIME_H
#define TVTIME_H

#include <QMainWindow>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QJsonObject>
#include <QShortcut>
#include <QTimer>

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
public slots:
    void beginSearch();

private slots:
    void on_searchButton_clicked();

    void on_addSeriesButton_clicked();

    void on_deleteSeriesButton_clicked();

    void on_searchResultsTableView_doubleClicked(const QModelIndex &index);

    void on_downloadMissingButton_clicked();

    void on_catalogDownloadsButton_clicked();

    void on_seriesTableWidget_clicked(const QModelIndex &index);

    void on_tabWidget_currentChanged(int index);

    void refresh_downloads();
private:
    QJsonDocument run_json_command( QStringList command );
    void add_series( const QString& id );
    void download_missing( const QString& id );
    void load_settings();
    void refresh_series( const QString& seriesid );
    void refresh_episodes( const QString& id );


    Ui::TVTime *ui;

    QJsonObject settings;

    QJsonTableModel *searchResults;

    QJsonTableModel *episodes;
    QJsonTableModel *downloads;

    QJsonObject seriesMap;
    QTimer *downloadTimer;
};

#endif // TVTIME_H
