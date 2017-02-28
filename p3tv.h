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

    void refresh_downloads();
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

    Ui::P3TV *ui;

    QJsonObject settings;

    QJsonTableModel *searchResults;

    QJsonTableModel *episodes;
    QJsonTableModel *downloads;

    QJsonObject seriesMap;
    QTimer *downloadTimer;
};

#endif // P3TV_H
