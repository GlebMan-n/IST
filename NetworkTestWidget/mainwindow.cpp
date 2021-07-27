#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include "kerbticket.h"
#include <QImage>
#include <QMap>
#include "photodownloaderi.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_execute_clicked()
{
    QMap<QString,QString> raw_headers;
    raw_headers["Host"] = "uzel.first.int";
    raw_headers["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64; rv:72.0) Gecko/20100101 Firefox/72.0";
    raw_headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8";
    raw_headers["Accept-Language"] = "ru-RU,ru;q=0.8,en-US;q=0.5,en;q=0.3";
    raw_headers["Accept-Encoding"] = "gzip, deflate";
    raw_headers["Connection"] = "keep-alive";
    raw_headers["Upgrade-Insecure-Requests"] = "1";
    raw_headers["Pragma"] = "no-cache";
    raw_headers["Cache-Control"] = "no-cache";
    int counter = 1;
    while (counter) {
        PhotoDownloader *downloader = new PhotoDownloader(ui->uid->text(), ui->main_url->text(), ui->kerb->text());
        connect(downloader,   SIGNAL(signDownloadFinished(QByteArray)),
                this,               SLOT(slotPicture_downloaded(QByteArray)));
        downloader->init(raw_headers);
        counter--;
    }

}

void MainWindow::slotPicture_downloaded(QByteArray ba)
{
    QImage image;
    bool bRes = image.loadFromData(ba);
    if(!bRes)
      qDebug() << "Cant load from data";
    ui->image->setPixmap(QPixmap::fromImage(image));
    PhotoDownloader *downloader = qobject_cast<PhotoDownloader*> (sender());
    downloader->deleteLater();
}


