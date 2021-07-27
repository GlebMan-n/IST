#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include "graphqlsubscriberpool.h"
#include <QTimer>
#include <QDir>

#define GRAPH_QL_IP "172.16.254.103"
#define GRAPH_QL_SETTINGS "GRAPH_QL_SETTINGS"
#define GRAPH_QL_PORT 8001
#define GRAPH_QL_ENDPOINT "subscriptions"
#define GRAPH_QL_QUERY_ADDR_BOOK_UPDATE "subscription{ addressBookUpdates{ data{ uuid firstName lastName patronymic email position { name groupUid } mssNumber meta } type { name }}}"
#define GRAPH_QL_QUERY_OSHS_UPDATE "subscription{oshsTreeUpdates{data {uuid parentUid description meta}type{ type name}}}"

int debug_counter;
int response_counter;
bool autoUP = true;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_sub_pool(0)
{
    settings_graphql_subscription_service settings;
    settings.m_endpoint = GRAPH_QL_ENDPOINT;
    settings.m_ip = GRAPH_QL_IP;
    settings.m_port = GRAPH_QL_PORT;

    m_sub_pool = new GraphQlsubscriberPool (settings,this);
    connect(m_sub_pool, SIGNAL(signSubscriptionData(int,QByteArray)),
            this,SLOT(slotData(int,QByteArray)));
    connect(m_sub_pool, SIGNAL(signSubscriptionMessage(int,QString)),
            this,SLOT(slotDebugMessage(int,QString)));
    connect(m_sub_pool, SIGNAL(signSubscriptionPayload(int,QString)),
            this,SLOT(slotOnGetPayload(int,QString)));
    connect(m_sub_pool, SIGNAL(signSubscriptionReady(int)),
            this,SLOT(slotOnConnectionReady(int)));
    connect(m_sub_pool, SIGNAL(signSubscriptionClose(int)),
            this,SLOT(slotOnConnectionClose(int)));
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{    
    delete ui;
}

void MainWindow::on_subscribe_pressed()
{
    QString query_text;
    query_text.append(GRAPH_QL_QUERY_ADDR_BOOK_UPDATE);
    query_text.append("\n");
    query_text.append(GRAPH_QL_QUERY_OSHS_UPDATE);
    ui->query->setPlainText(query_text);
    m_sub_pool->addSubscription(GRAPH_QL_QUERY_ADDR_BOOK_UPDATE);
    m_sub_pool->addSubscription(GRAPH_QL_QUERY_OSHS_UPDATE);
    const QList<int> &keys = m_sub_pool->getSubscriptions().keys();
    for (unsigned i = 0; i < keys.size(); i++)
        m_sub_pool->askToSubscribe(keys.at(i));
}

void MainWindow::on_unsubscribe_pressed()
{
    m_sub_pool->removeAllSubscribes();
}

void MainWindow::on_clear_pressed()
{
    ui->response->clear();
    ui->debug->clear();
}

void MainWindow::slotData(int id, QByteArray ba)
{
    const QString &old_text = ui->response->toPlainText();
    QString new_text = QString::number(id) + " - " + ba +  "\n" + old_text;
    ui->response->setPlainText(new_text);
}

void MainWindow::slotDebugMessage(int id , QString message)
{
    const QString &old_text = ui->debug->toPlainText();
    QString new_text = QString::number(debug_counter++) + " " + QString::number(id) + " - " + message +  "\n" + old_text;
    ui->debug->setPlainText(new_text);
}

void MainWindow::slotOnGetPayload(int id, QString payload)
{
    const QString &old_text = ui->response->toPlainText();
    QString new_text = QString::number(id) + " : " + payload + "\n"+ old_text;
    ui->response->setPlainText(new_text);
}

void MainWindow::slotOnConnectionReady(int id)
{
    const QString &old_text = ui->debug->toPlainText();
    QString new_text = QString::number(id) + " - " + " - ready" +  "\n" + old_text;
    ui->debug->setPlainText(new_text);
}

void MainWindow::slotOnConnectionClose(int id)
{
    const QString &old_text = ui->debug->toPlainText();
    QString new_text = QString::number(id) + " - " + " - closed" +  "\n" + old_text;
    ui->debug->setPlainText(new_text);
}

