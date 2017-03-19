#ifndef P3TV_H
#define P3TV_H

#include <QMainWindow>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QJsonObject>
#include <QShortcut>
#include <QTimer>

#include "qjsontablemodel.h"

namespace Ui {
class P3TV;
}

class WebRunner;

class P3TV : public QMainWindow
{
    Q_OBJECT

public:
    explicit P3TV(QWidget *parent = 0);
    ~P3TV();
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


    void enable_refresh_downloads();
    void enable_download_available();
    void enable_update_episode_cache();
    void do_tasks();
    void on_revertPushButton_clicked();

    void on_savePushButton_clicked();

private:
    QJsonDocument run_json_command( QStringList command );
    void add_series( const QString& id );
    void download_missing( const QString& id );
    void load_settings();
    void refresh_series( const QString& seriesid );
    void refresh_episodes( const QString& id );
    void load_form_data();
    void save_setting(const QString& setting_key, const QString& setting_value );


    void refresh_downloads();
    void download_available();
    void update_episode_cache();


    Ui::P3TV *ui;

    QJsonObject settings;
    QJsonObject seriesMap;
    QJsonArray seriesArray;

    QJsonTableModel *searchResults;

    QJsonTableModel *episodes;
    QJsonTableModel *downloads;



    bool m_bRefreshDownloads, m_bDownloadAvailable, m_bUpdateEpisodeCache;
    int m_iDownloadAvailableIndex, m_iUpdateEpisodeCacheIndex;
    QTimer *downloadsTimer; // updates download status, catalogs downloaded, clears 100% from transmission
    QTimer *downloadAvailableTimer; // cycles through series and checks for available episodes
    QTimer *updateEpisodeCacheTimer; // cycles through series and updates episodes.json cache
    QTimer *taskTimer;

    WebRunner *webRunner;
};

#endif // P3TV_H
