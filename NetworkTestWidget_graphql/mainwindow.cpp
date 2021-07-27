#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QUuid>
#include "HttpClientQt.h"
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_client = new HttpClientQt(this);
    m_webSocket = NULL;
    ui->subscribtion->hide();
    connect(m_client,SIGNAL(signData(const QByteArray&)),this,SLOT(on_data_sended(const QByteArray&)));
    update_query_list();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_execute_clicked()
{
    const QString &url_main = ui->main_url->text();
    const QString &url_postfix = ui->postfix->text();
    const QString &port = ui->port->text();
    QString url_whole = url_main + ":" + port + "/" + url_postfix;
    QByteArray ba = ui->query->currentText().toLocal8Bit();
    QMap<QString,QString> raw_headers;
    raw_headers["User-Agent"] = "curl/7.58.0";
    raw_headers["Accept"] = "*/*";
    raw_headers["Content-Type"] = "application/json";
    raw_headers["Content-length"] = QString::number(ba.length());
    m_client->sendData(url_whole, raw_headers, ba);
}

void MainWindow::update_query_combo_box()
{
    ui->query->blockSignals(true);
    ui->query->clear();
    ui->query->addItems(m_list_queries);
    ui->query->setCurrentIndex(m_list_queries.size() - 1);
    ui->query->blockSignals(false);
}

void MainWindow::update_query_list()
{
    m_list_queries.clear();
    for (unsigned i = 0; i < (unsigned int)ui->query->count(); i++)
        m_list_queries.append(ui->query->itemText(i));
}

void MainWindow::on_clear_clicked()
{
   ui->response->clear();
}

void MainWindow::setResponseText(const QString &string)
{
    ui->response->setText(string);
}

void MainWindow::on_data_sended(const QByteArray &data)
{
    ui->response->setText(data);
}

void MainWindow::on_query_editTextChanged(const QString &text)
{
    if (!m_block_changes) {
        m_list_queries[ui->query->currentIndex()] = text;
        update_query_combo_box();
        m_block_changes = true;
    }
}

void MainWindow::on_addQuery_pressed()
{
    bool ok;
        QString new_query = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                             tr("new query:"), QLineEdit::Normal,
                                             "{ \"query\": \"{ your_text_here }\" }", &ok);
        if (ok && !new_query.isEmpty()) {
            m_list_queries.append(new_query);
            update_query_combo_box();
        }
}

void MainWindow::on_query_currentIndexChanged(int i)
{
    Q_UNUSED(i);
}

void MainWindow::on_del_pressed()
{
    ui->query->removeItem(ui->query->currentIndex());
    update_query_list();
}


