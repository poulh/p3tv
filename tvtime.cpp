#include "tvtime.h"
#include "ui_tvtime.h"

#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>



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
        header.push_back( QJsonTableModel::Heading( { {"title","Rating"},   {"index","rating"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","ID"},   {"index","id"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Title"},   {"index","name"} }) );


        series = new QJsonTableModel( header, this );
        ui->seriesTableView->setModel( series );
        ui->seriesTableView->horizontalHeader()->setStretchLastSection(true);
        ui->seriesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
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
        ui->episodeTableView->horizontalHeader()->setStretchLastSection(true);
        ui->episodeTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    }

    connect(ui->searchLineEdit, SIGNAL(returnPressed()),ui->searchButton,SIGNAL(clicked()));

    refresh_series();
}

TVTime::~TVTime()
{
    delete ui;
}


QJsonDocument TVTime::run_json_command( QStringList command )
{
    qDebug() << command;

    QProcess process;
    process.start("/usr/local/bin/tvtime_json_api", command);

    process.waitForFinished(-1); // will wait forever until finished

    QString stdout = process.readAllStandardOutput();
    QString stderr = process.readAllStandardError();
    qDebug() << stdout;
    qDebug() << stderr;
    qDebug() << "--------------------";
    return QJsonDocument::fromJson(stdout.toUtf8());
};


void TVTime::load_settings()
{
    QStringList args;
    args << "settings";

    QJsonDocument jsonDocument = run_json_command( args );
    settings = jsonDocument.object();
}

void TVTime::refresh_series()
{
    load_settings();
    QJsonArray array = settings["series"].toArray();
    series->setJson( array );
}


void TVTime::on_searchButton_clicked()
{
    searchResults->removeRows(0,searchResults->rowCount());
    QStringList args;
    args << "search";
    args << ui->searchLineEdit->text();
    QJsonDocument jsonDocument = run_json_command( args );
    searchResults->setJson( jsonDocument );
}

void TVTime::on_addSeriesButton_clicked()
{
    QModelIndexList list = ui->searchResultsTableView->selectionModel()->selectedIndexes();
    foreach( const QModelIndex &index, list )
    {
        QJsonObject object = searchResults->getJsonObject( index );
        QStringList args;
        args << "add_series";
        args << object["id"].toString();
        QJsonDocument jsonDocument = run_json_command( args );
    }
    refresh_series();
}


void TVTime::on_deleteSeriesButton_clicked()
{
    QModelIndexList list = ui->seriesTableView->selectionModel()->selectedIndexes();
    foreach( const QModelIndex &index, list )
    {
        QJsonObject object = series->getJsonObject( index );
        QStringList args;
        args << "remove_series";
        args << object["id"].toString();
        QJsonDocument jsonDocument = run_json_command( args );
    }
    refresh_series();
}

void TVTime::on_seriesTableView_clicked(const QModelIndex &index)
{
    QJsonObject object = series->getJsonObject( index );


    QStringList args;
    args << "episode_status";
    args << object["id"].toString();

    QJsonDocument jsonDocument = run_json_command( args );
    episodes->setJson( jsonDocument );
    ui->episodeTableView->setFocus();
}

void TVTime::on_searchResultsTableView_doubleClicked(const QModelIndex &index)
{
    QJsonObject object = searchResults->getJsonObject( index );

    QStringList args;
    args << "search_and_add";
    args << object["id"].toString();
    QJsonDocument jsonDocument = run_json_command( args );
    refresh_series();
    ui->seriesTableView->setFocus();
}


void TVTime::on_downloadMissingButton_clicked()
{
    QModelIndexList list = ui->seriesTableView->selectionModel()->selectedIndexes();
    foreach( const QModelIndex &index, list )
    {
        QJsonObject object = series->getJsonObject( index );

        QStringList args;
        args << "download_missing";
        args << object["id"].toString();
        QJsonDocument jsonDocument = run_json_command( args );
        on_seriesTableView_clicked( index ); //refresh episode list
        break;
    }
}

void TVTime::on_catalogDownloadsButton_clicked()
{
    QModelIndexList list = ui->seriesTableView->selectionModel()->selectedIndexes();
    foreach( const QModelIndex &index, list )
    {
        QJsonObject object = series->getJsonObject( index );

        QStringList args;
        args << "catalog_downloads";
        args << object["id"].toString();
        QJsonDocument jsonDocument = run_json_command( args );
        on_seriesTableView_clicked( index ); //refresh episode list
        break;
    }
}
