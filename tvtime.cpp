#include "tvtime.h"
#include "ui_tvtime.h"

#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalMapper>
#include <QProgressBar>

TVTime::TVTime(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TVTime)
{
    ui->setupUi(this);

    {
        QJsonTableModel::Header header;


        header.push_back( QJsonTableModel::Heading( { {"title","Network"},   {"index","network"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Rating"},   {"index","rating"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Title"},   {"index","name"} }) );

        searchResults = new QJsonTableModel( header, this );
        ui->searchResultsTableView->setModel( searchResults );
        ui->searchResultsTableView->horizontalHeader()->setStretchLastSection(true);
        ui->searchResultsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
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


    connect(ui->searchLineEdit, SIGNAL(returnPressed()),ui->searchButton,SIGNAL(clicked()));

    ui->seriesTableWidget->horizontalHeader()->setVisible(false);
    ui->seriesTableWidget->verticalHeader()->setVisible(false);

    ui->seriesTableWidget->setRowCount(4);
    ui->seriesTableWidget->setSelectionBehavior(QAbstractItemView::SelectColumns);
    // image row
    ui->seriesTableWidget->setRowHeight(0,200);


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


    refresh_series();
}

TVTime::~TVTime()
{
    delete ui;
}


QJsonDocument TVTime::run_json_command( QStringList command )
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    qDebug() << command;

    QProcess process;
    process.start("/usr/local/bin/tvtime_json_api", command);

    process.waitForFinished(-1); // will wait forever until finished

    QString stdout = process.readAllStandardOutput();
    QString stderr = process.readAllStandardError();
    qDebug() << stdout;
    qDebug() << stderr;
    qDebug() << "--------------------";
    QApplication::restoreOverrideCursor();
    return QJsonDocument::fromJson(stdout.toUtf8());
};


void TVTime::load_settings()
{
    QStringList args;
    args << "settings";

    QJsonDocument jsonDocument = run_json_command( args );
    settings = jsonDocument.object();

    foreach( const QJsonValue& value, settings["series"].toArray() )
    {
        QJsonObject theSeries = value.toObject();
        seriesMap.insert( theSeries["id"].toString() , value );

    }
}

void TVTime::refresh_series()
{
    load_settings();
    QJsonArray array = settings["series"].toArray();


    ui->seriesTableWidget->setColumnCount( array.size() );
    uint column = 0;
    foreach( const QJsonValue& v, array )
    {
        QJsonObject theSeries = v.toObject();

        ui->seriesTableWidget->setColumnWidth(column,136);

        QJsonObject banners = theSeries["banners"].toObject();
        QString posterPath = banners["poster"].toString();
        QImage img(posterPath); QTableWidgetItem *thumbnail = new QTableWidgetItem;
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


        ++column;
    }
    ui->tabWidget->setCurrentIndex(0);
}

void TVTime::refresh_downloads()
{
    QStringList args;
    args << "catalog_and_downloads";

    QJsonDocument jsonDocument = run_json_command( args );

    QJsonArray downloads = jsonDocument.array();
    ui->downloadsTableWidget->setRowCount( downloads.size() );
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

}


void TVTime::on_searchButton_clicked()
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

void TVTime::on_addSeriesButton_clicked()
{
    QModelIndexList list = ui->searchResultsTableView->selectionModel()->selectedIndexes();
    if( list.empty() )
    {
        return;
    }

    QModelIndex index = list[0];
    QJsonObject object = searchResults->getJsonObject( index );
    QStringList args;
    args << "add_series";
    args << object["id"].toString();
    QJsonDocument jsonDocument = run_json_command( args );

    refresh_series();
}


void TVTime::on_deleteSeriesButton_clicked()
{
    QModelIndexList list = ui->seriesTableWidget->selectionModel()->selectedIndexes();

    foreach( const QModelIndex &index, list )
    {
        QStringList args;
        args << "remove_series";
        QString id = ui->seriesTableWidget->item(0, index.column())->text();
        args << id;
        QJsonDocument jsonDocument = run_json_command( args );
        break;
    }
   refresh_series();
}

void TVTime::on_searchResultsTableView_doubleClicked(const QModelIndex &index)
{
    QJsonObject object = searchResults->getJsonObject( index );

    QStringList args;
    args << "add_series";
    args << object["id"].toString();
    QJsonDocument jsonDocument = run_json_command( args );
    refresh_series();

}


void TVTime::on_downloadMissingButton_clicked()
{
    QModelIndexList list = ui->seriesTableWidget->selectionModel()->selectedIndexes();
    if( list.empty() )
    {
        return;
    }

    QModelIndex index = list[0];
    QStringList args;
    args << "download_missing";
    QString id = ui->seriesTableWidget->item(0, index.column())->text();
    args << id;
    QJsonDocument jsonDocument = run_json_command( args );

    on_seriesTableWidget_clicked( index ); // refresh episode list
}

void TVTime::on_catalogDownloadsButton_clicked()
{
    QModelIndexList list = ui->seriesTableWidget->selectionModel()->selectedIndexes();
    if( list.empty() )
    {
        return;
    }

    QModelIndex index = list[0];
    QStringList args;
    args << "catalog_downloads_for_series";
    QString id = ui->seriesTableWidget->item(0, index.column())->text();
    args << id;
    QJsonDocument jsonDocument = run_json_command( args );

    on_seriesTableWidget_clicked( index ); // refresh episode list
}

void TVTime::on_seriesTableWidget_clicked(const QModelIndex &index)
{
    QJsonArray series = settings["series"].toArray();
    QJsonObject selectedSeries = series[ index.column() ].toObject();
    QString id = selectedSeries["id"].toString();

    QStringList args;
    args << "episode_status";
    args << id;

    QJsonDocument jsonDocument = run_json_command( args );
    episodes->setJson( jsonDocument );
    ui->episodeTableView->setFocus();

}

void TVTime::on_tabWidget_currentChanged(int index)
{
    switch( index )
    {
    case 1:
        refresh_downloads();
        break;
    case 2:
        ui->searchLineEdit->setFocus();
        ui->searchLineEdit->selectAll();
        break;
    }
}

void TVTime::beginSearch()
{
    qDebug() << "begin search";
}

