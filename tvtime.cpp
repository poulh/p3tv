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

        header.push_back( QJsonTableModel::Heading( { {"title","IMDB ID"}, {"index","imdb_id"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Title"},   {"index","title"} }) );

        searchResults = new QJsonTableModel( header, this );
        ui->searchResultsTableView->setModel( searchResults );
        ui->searchResultsTableView->horizontalHeader()->setStretchLastSection(true);
        ui->searchResultsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    }

    {
        QJsonTableModel::Header header;
        header.push_back( QJsonTableModel::Heading( { {"title","Title"},   {"index","title"} }) );

        series = new QJsonTableModel( header, this );
        ui->seriesTableView->setModel( series );
        ui->seriesTableView->horizontalHeader()->setStretchLastSection(true);
    }

    {
        QJsonTableModel::Header header;
        header.push_back( QJsonTableModel::Heading( { {"title","Title"},    {"index","title"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Season"},   {"index","season"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Episode"},  {"index","episode"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Air Date"}, {"index","air_date"} }) );
        header.push_back( QJsonTableModel::Heading( { {"title","Path"},     {"index","path"} }) );
        episodes = new QJsonTableModel( header, this );
        ui->episodeTableView->setModel( episodes );
        ui->episodeTableView->horizontalHeader()->setStretchLastSection(true);
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
    QProcess process;
    process.start("/usr/local/bin/tvtime_json_api", command);
    process.waitForFinished(-1); // will wait forever until finished

    QString stdout = process.readAllStandardOutput();
    qDebug() << stdout;
    return QJsonDocument::fromJson(stdout.toUtf8());
};

void TVTime::refresh_series()
{
    series->removeRows(0,series->rowCount());
    QStringList args;
    args << "series";
    QJsonDocument jsonDocument = run_json_command( args );
    series->setJson( jsonDocument );
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
    foreach( const QModelIndex &item, list )
    {
        QStringList args;
        args << "search_and_add";
        args << item.data().toString();
        QJsonDocument jsonDocument = run_json_command( args );
    }
    refresh_series();
}


void TVTime::on_deleteSeriesButton_clicked()
{
    QModelIndexList list = ui->seriesTableView->selectionModel()->selectedIndexes();
    foreach( const QModelIndex &item, list )
    {
        QStringList args;
        args << "remove_series";
        args << item.data().toString();
        QJsonDocument jsonDocument = run_json_command( args );
    }
    refresh_series();
}

void TVTime::on_seriesTableView_clicked(const QModelIndex &index)
{
    QJsonObject object = series->getJsonObject( index );


    QStringList args;
    args << "search_and_list";
    args << object["title"].toString();

    QJsonDocument jsonDocument = run_json_command( args );
    episodes->setJson( jsonDocument );
}
