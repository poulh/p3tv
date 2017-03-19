#include "p3tv.h"
#include "ui_p3tv.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalMapper>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QUrlQuery>

#include "webrunner.h"

const int DISABLED = -1;

P3TV::P3TV(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::P3TV)
    , m_bDownloadAvailable( false )
    , m_bRefreshDownloads( true )
    , m_bUpdateEpisodeCache( false )
    , m_iDownloadAvailableIndex( 0 )
    , m_iUpdateEpisodeCacheIndex( 0 )
    , webRunner( new WebRunner( this ) )
{
    ui->setupUi(this);
    webRunner->start();
    QThread::sleep(1);

    {
        QJsonTableModel::Header header;


        header.push_back( QJsonTableModel::Heading( { {"title","Network"},   {"index","network"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Rating"},   {"index","rating"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Title"},   {"index","name"} }) );

        searchResults = new QJsonTableModel( header, this );
        ui->searchResultsTableView->setModel( searchResults );
        ui->searchResultsTableView->horizontalHeader()->setStretchLastSection(true);
        ui->searchResultsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        connect(ui->searchLineEdit, SIGNAL(returnPressed()),ui->searchButton,SIGNAL(clicked()));
    }

    {
        QJsonTableModel::Header header;
        header.push_back( QJsonTableModel::Heading( { {"title","Title"},    {"index","title"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Season"},   {"index","season"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Episode"},  {"index","episode"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Air Date"}, {"index","air_date"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Status"},     {"index","status"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","% Done"},     {"index","percent_done"} }) );
        episodes = new QJsonTableModel( header, this );
        ui->episodeTableView->setModel( episodes );


        ui->episodeTableView->setColumnWidth(0, this->width() * 0.44 );
        ui->episodeTableView->setColumnWidth(1, this->width() * 0.1 );
        ui->episodeTableView->setColumnWidth(2, this->width() * 0.1 );
        ui->episodeTableView->setColumnWidth(3, this->width() * 0.1 );
        ui->episodeTableView->setColumnWidth(4, this->width() * 0.1 );
        ui->episodeTableView->setColumnWidth(5, this->width() * 0.1 );
    //    ui->episodeTableView->horizontalHeader()->setStretchLastSection(true);
        ui->episodeTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    }

    {
        ui->downloadsTableWidget->setColumnCount(2);
        ui->downloadsTableWidget->setColumnWidth(0, 68);
        ui->downloadsTableWidget->horizontalHeader()->setVisible(false);
        ui->downloadsTableWidget->verticalHeader()->setVisible(false);
        ui->downloadsTableWidget->horizontalHeader()->setStretchLastSection(true);
        ui->downloadsTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    }

    {
        ui->seriesTableWidget->horizontalHeader()->setVisible(false);
        ui->seriesTableWidget->verticalHeader()->setVisible(false);

        ui->seriesTableWidget->setRowCount(4);
        ui->seriesTableWidget->setSelectionBehavior(QAbstractItemView::SelectColumns);
        // image row
        ui->seriesTableWidget->setRowHeight(0,200);
    }

    {
        downloadsTimer = new QTimer( this );
        connect( downloadsTimer, SIGNAL( timeout() ), this, SLOT( enable_refresh_downloads() ) );
        downloadsTimer->start( 10 * 1000 );
    }

    {
        downloadAvailableTimer = new QTimer( this );
        connect( downloadAvailableTimer, SIGNAL( timeout() ), this, SLOT( enable_download_available() ) );
        downloadAvailableTimer->start( 60 * 60 * 1000 );
    }

    {
        updateEpisodeCacheTimer = new QTimer( this );
        connect( updateEpisodeCacheTimer, SIGNAL( timeout() ), this, SLOT( enable_update_episode_cache() ) );
        updateEpisodeCacheTimer->start( 24 * 60 * 60 * 1000 );
    }

    {
        taskTimer = new QTimer( this );
        connect( taskTimer, SIGNAL( timeout() ), this, SLOT( do_tasks() ) );
        taskTimer->start( 1000 );
    }
    //http://stackoverflow.com/questions/10160232/qt-designer-shortcut-to-another-tab
    // Setup a signal mapper to avoid creating custom slots for each tab
    QSignalMapper *m = new QSignalMapper(this);

    // Setup the shortcut for the first tab
    QShortcut *s0 = new QShortcut(QKeySequence("Ctrl+b"), this);
    connect(s0, SIGNAL(activated()), m, SLOT(map()));
    m->setMapping(s0, 0);

    QShortcut *s1 = new QShortcut(QKeySequence("Ctrl+d"), this);
    connect(s1, SIGNAL(activated()), m, SLOT(map()));
    m->setMapping(s1, 1);

    // Setup the shortcut for the second tab
    QShortcut *s2 = new QShortcut(QKeySequence::Find, this);
    connect(s2, SIGNAL(activated()), m, SLOT(map()));
    m->setMapping(s2, 2);

    // Wire the signal mapper to the tab widget index change slot
    connect(m, SIGNAL(mapped(int)), ui->tabWidget, SLOT(setCurrentIndex(int)));

    refresh_series( "" );
qDebug() << "end construct";
}

P3TV::~P3TV()
{
    delete ui;

    taskTimer->stop();

    QStringList args;
    args << "exit";

    run_json_command( args );

    webRunner->wait();
}

void P3TV::enable_refresh_downloads()
{
    m_bRefreshDownloads = true;
}


void P3TV::enable_download_available()
{
    m_iDownloadAvailableIndex = 0;
}


void P3TV::enable_update_episode_cache()
{
    m_iUpdateEpisodeCacheIndex = 0;
}


void P3TV::do_tasks()
{
    if( m_bRefreshDownloads )
    {
        refresh_downloads();
    }
    else if( m_iDownloadAvailableIndex != DISABLED )
    {
        download_available();
    }
    else if( m_iUpdateEpisodeCacheIndex != DISABLED )
    {
        update_episode_cache();
    }
}


QJsonDocument P3TV::run_json_command( QStringList command )
{

    QApplication::setOverrideCursor(Qt::WaitCursor);
    qDebug() << command;

    QNetworkAccessManager NAManager;
    NAManager.setNetworkAccessible(QNetworkAccessManager::Accessible); // <--


    QUrl qurl( "http://0.0.0.0/" + command[0] );
    QUrlQuery query;
    if( command.count() > 1 )
    {
        query.addQueryItem("key", command[1]);
    }
    if( command.count() > 2 )
    {
        query.addQueryItem("val", command[2]);
    }
    qurl.setPort(4567);
    qurl.setQuery( query );
    qDebug() << qurl.toString();
    QNetworkRequest request( qurl );

    QNetworkReply *reply = NAManager.get( request );
    QEventLoop eventLoop;
    QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();
    if (reply->error() == QNetworkReply::NoError)
    {
        QString json( reply->readAll() );
        QApplication::restoreOverrideCursor();
        return QJsonDocument::fromJson( json.toUtf8() );
    }

    QApplication::restoreOverrideCursor();
    return QJsonDocument();
}

void P3TV::load_settings()
{
    QStringList args;
    args << "settings";

    QJsonDocument jsonDocument = run_json_command( args );
    settings = jsonDocument.object();
    seriesArray = settings["series"].toArray();

    foreach( const QJsonValue& value, seriesArray )
    {
        QJsonObject theSeries = value.toObject();
        seriesMap.insert( theSeries["id"].toString() , value );

    }
    load_form_data();
}

void P3TV::load_form_data()
{
    ui->downloadPathLineEdit->setText( settings["download_path"].toString() );
    ui->libraryPathLineEdit->setText( settings["library_path"].toString() );

    ui->transmissionUsernameLineEdit->setText( settings["transmission_username"].toString() );
    ui->transmissionPasswordLineEdit->setText( settings["transmission_password"].toString() );
    ui->transmissionHostLineEdit->setText( settings["transmission_host"].toString() );
    ui->transmissionPortLineEdit->setText( QString::number( settings["transmission_port"].toDouble() ) );
}

void P3TV::save_setting(const QString& setting_key, const QString& setting_value )
{
    if( settings[ setting_key ].toString() != setting_value )
    {
        QStringList args;
        args << "set";
        args << setting_key;
        args << setting_value;
        QJsonDocument jsonDocument = run_json_command( args );
    }
}

void P3TV::on_savePushButton_clicked()
{
    save_setting( "download_path", ui->downloadPathLineEdit->text() );
    save_setting( "library_path", ui->libraryPathLineEdit->text() );
    save_setting( "transmission_username", ui->transmissionUsernameLineEdit->text() );
    save_setting( "transmission_password", ui->transmissionPasswordLineEdit->text() );
    save_setting( "transmission_host", ui->transmissionHostLineEdit->text() );
    save_setting( "transmission_port", ui->transmissionPortLineEdit->text() );

    load_settings();
}

void P3TV::refresh_series(const QString &seriesid )
{
    load_settings();

    ui->seriesTableWidget->setColumnCount( seriesArray.size() );
    uint column = 0;
    foreach( const QJsonValue& v, seriesArray )
    {
        QJsonObject theSeries = v.toObject();

        ui->seriesTableWidget->setColumnWidth(column,136);

        QJsonObject banners = theSeries["banners"].toObject();
        QString posterPath = banners["poster"].toString();
        QImage img(posterPath);
        QTableWidgetItem *thumbnail = new QTableWidgetItem;
        QString title = theSeries["name"].toString();
        thumbnail->setToolTip( title );
        thumbnail->setData(Qt::DecorationRole, QPixmap::fromImage( img ).scaled(136,200) );
        ui->seriesTableWidget->setItem(0,column,thumbnail);

        QTableWidgetItem* network = new QTableWidgetItem;
        network->setText( theSeries["network"].toString() );
        ui->seriesTableWidget->setItem(1,column, network);


        QTableWidgetItem* rating = new QTableWidgetItem;
        rating->setText( QString::number( theSeries["rating"].toDouble() ) );
        ui->seriesTableWidget->setItem(2,column, rating);


        QTableWidgetItem* contentRating = new QTableWidgetItem;
        contentRating->setText( theSeries["content_rating"].toString() );
        ui->seriesTableWidget->setItem(3,column, contentRating);

        if( theSeries["id"].toString() == seriesid )
        {
            ui->seriesTableWidget->selectColumn( column );
            refresh_episodes( seriesid );
        }


        ++column;
    }
}

void P3TV::refresh_downloads()
{
    QStringList args;
    args << "catalog_and_downloads";

    QJsonDocument jsonDocument = run_json_command( args );

    QJsonArray downloads = jsonDocument.array();
    ui->downloadsTableWidget->setRowCount( downloads.size() );

    if( downloads.size() > 0 )
    {
        ui->tabWidget->setTabText( 1, "Downloads ( " + QString::number( downloads.size()) + " )" );
    }
    else
    {
        ui->tabWidget->setTabText( 1, "Downloads" );
    }

    uint row = 0;
    foreach( const QJsonValue& value, downloads )
    {
        QJsonObject download = value.toObject();
        QJsonObject theSeries = seriesMap[ download["series_id"].toString() ].toObject();
        QJsonObject banners = theSeries["banners"].toObject();
        QString posterPath = banners["poster"].toString();

        ui->downloadsTableWidget->setRowHeight(row,100);

        QImage img(posterPath); QTableWidgetItem *thumbnail = new QTableWidgetItem;

        thumbnail->setToolTip( download["series"].toString() );
        thumbnail->setData(Qt::DecorationRole, QPixmap::fromImage( img ).scaled(68,100) );
        ui->downloadsTableWidget->setItem(row,0,thumbnail);


        QTableWidget *progressTable = new QTableWidget(2,1);
        progressTable->setSelectionMode(QAbstractItemView::NoSelection);
        progressTable->horizontalHeader()->setVisible(false);
        progressTable->verticalHeader()->setVisible(false);
        progressTable->horizontalHeader()->setStretchLastSection(true);
        progressTable->setRowHeight(0,50-2);
        progressTable->setRowHeight(1,50-2);

        QTableWidgetItem* progressHeader = new QTableWidgetItem;
        QString header = download["series"].toString();
        header += " - S" + QString::number( download["season"].toDouble() ).rightJustified(2,'0');
        header += "E" + QString::number( download["episode"].toDouble() ).rightJustified(2,'0');
        header += " - " + download["path"].toString();
        progressHeader->setText( header );

        QProgressBar *progress = new QProgressBar();
        progress->setRange(0,1000);
        progress->setFormat("%v%");
        progress->setTextVisible( true );
        progress->setValue( download["percent_done"].toDouble() * 1000 );

        progressTable->setItem( 0, 0, progressHeader );
        progressTable->setCellWidget( 1, 0, progress );

        ui->downloadsTableWidget->setCellWidget(row,1, progressTable);

        ++row;
    }

    m_bRefreshDownloads = false;
}

void P3TV::update_episode_cache()
{
    if( m_iUpdateEpisodeCacheIndex < seriesArray.size() )
    {
        const QJsonObject obj = seriesArray[ m_iUpdateEpisodeCacheIndex ].toObject();

        QString id = obj["id"].toString();
        qDebug() << m_iUpdateEpisodeCacheIndex << " " << seriesArray.size();
        QStringList args;
        args << "update_series";
        args << id;
        QJsonDocument jsonDocument = run_json_command( args );

        ++m_iUpdateEpisodeCacheIndex;
    }
    if( m_iUpdateEpisodeCacheIndex >= seriesArray.size() )
    {
        m_iUpdateEpisodeCacheIndex = DISABLED;
    }
}

void P3TV::download_available()
{
    if( m_iDownloadAvailableIndex < seriesArray.size() )
    {
        const QJsonObject obj = seriesArray[ m_iDownloadAvailableIndex ].toObject();

        QString id = obj["id"].toString();
        qDebug() << m_iDownloadAvailableIndex << " " << seriesArray.size();
        download_missing( id );

        ++m_iDownloadAvailableIndex;
    }
    if( m_iDownloadAvailableIndex >= seriesArray.size() )
    {
        m_iDownloadAvailableIndex = DISABLED;
    }
}

void P3TV::on_searchButton_clicked()
{
    searchResults->removeRows(0,searchResults->rowCount());
    QStringList args;
    args << "search";
    args << ui->searchLineEdit->text();
    QJsonDocument jsonDocument = run_json_command( args );
    searchResults->setJson( jsonDocument );

    ui->searchLineEdit->setFocus();
    ui->searchLineEdit->selectAll();
}

void P3TV::on_addSeriesButton_clicked()
{
    QModelIndexList list = ui->searchResultsTableView->selectionModel()->selectedIndexes();
    if( list.empty() )
    {
        return;
    }

    QModelIndex index = list[0];
    on_searchResultsTableView_doubleClicked( index ); //add series and refresh e
}

void P3TV::add_series( const QString& id )
{
    QStringList args;
    args << "add_series";
    args << id;
    QJsonDocument jsonDocument = run_json_command( args );
}


void P3TV::on_deleteSeriesButton_clicked()
{
    QModelIndexList list = ui->seriesTableWidget->selectionModel()->selectedIndexes();

    if( list.empty() )
    {
        return;
    }

    QModelIndex index = list[0];
    QJsonObject selectedSeries = seriesArray[ index.column() ].toObject();
    QString id = selectedSeries["id"].toString();

    QStringList args;
    args << "remove_series";
    args << id;
    QJsonDocument jsonDocument = run_json_command( args );

    QJsonObject nextSeries = seriesArray[ std::min( index.column(), seriesArray.size() -2 ) ].toObject();
    refresh_series( nextSeries["id"].toString() );
}

void P3TV::on_searchResultsTableView_doubleClicked(const QModelIndex &index)
{
    QJsonObject object = searchResults->getJsonObject( index );
    QString id = object["id"].toString();
    add_series( id);
    download_missing( id );
    refresh_downloads();
    refresh_series( id );
    ui->tabWidget->setCurrentIndex(0);
}


void P3TV::on_downloadMissingButton_clicked()
{
    QModelIndexList list = ui->seriesTableWidget->selectionModel()->selectedIndexes();
    if( list.empty() )
    {
        return;
    }

    QModelIndex index = list[0];
    QJsonObject selectedSeries = seriesArray[ index.column() ].toObject();
    QString id = selectedSeries["id"].toString();

    download_missing( id );

    refresh_series( id );
}

void P3TV::download_missing( const QString& id )
{
    QStringList args;
    args << "download_missing";
    args << id;
    QJsonDocument jsonDocument = run_json_command( args );
}

void P3TV::on_catalogDownloadsButton_clicked()
{
    QModelIndexList list = ui->seriesTableWidget->selectionModel()->selectedIndexes();
    if( list.empty() )
    {
        return;
    }

    QModelIndex index = list[0];
    QJsonObject selectedSeries = seriesArray[ index.column() ].toObject();
    QString id = selectedSeries["id"].toString();

    QStringList args;
    args << "catalog_downloads_for_series";
    args << id;
    QJsonDocument jsonDocument = run_json_command( args );

    refresh_series( id );
}

void P3TV::on_seriesTableWidget_clicked(const QModelIndex &index)
{
    QJsonObject selectedSeries = seriesArray[ index.column() ].toObject();
    QString id = selectedSeries["id"].toString();

    refresh_episodes( id );
}

void P3TV::refresh_episodes( const QString& id )
{
    QStringList args;
    args << "episode_status";
    args << id;

    QJsonDocument jsonDocument = run_json_command( args );
    episodes->setJson( jsonDocument );
    ui->episodeTableView->setFocus();
}

void P3TV::on_tabWidget_currentChanged(int index)
{
    QString tabName = ui->tabWidget->tabText( index );
    if( tabName == "Search" )
    {
        ui->searchLineEdit->setFocus();
        ui->searchLineEdit->selectAll();
    }
    else if( tabName == "Browse")
    {

    }
    else // Downloads
    {
        //refresh_downloads();
    }
}

void P3TV::beginSearch()
{
    qDebug() << "begin search";
}


void P3TV::on_revertPushButton_clicked()
{
    load_form_data();
}


