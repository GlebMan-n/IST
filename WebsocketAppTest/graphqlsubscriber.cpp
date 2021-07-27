#include "graphqlsubscriber.h"
#include "nopoll_conn.h"
#include "nopoll_listener.h"
#include "graphqlsubscriberpool.h"
#include "limits.h"

#include <QDebug>

#define READ_TIME_OUT 500
#define SLEEP 50000
#define WAIT_UNTIL_CONNECT 10
#define PING_FAIL_COUNT 3

#include <QTimer>

//здесь указатель на пул подписок
static QObject* d = 0;

//колл бэк принимает указатель на контекст, сможем передать в пул и разобраться от кого пришло сообщение
//пригодится для автоматической активации подписок после разрыва соединения в том числе
static void messageToSubscriber(const MESSAGE_TYPE &type, const char* message, noPollCtx *ctx)
{
    GraphQlsubscriberPool* subscriber_pool = 0;
    if (d)
        subscriber_pool = qobject_cast<GraphQlsubscriberPool*>(d);
    if (!subscriber_pool)
        return;

    if (type == CONNECTION_ON_CLOSE)
        subscriber_pool->on_connection_closed((void*)ctx);

    //позже уберу и все сделаю через пул
    GraphQlSubscriber * subscriber = subscriber_pool->getSubscriberByCondext((void*)ctx);
    if (subscriber)
        subscriber->setMessage(type, message);
}

nopoll_bool __ctx_on_accept_handler(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data)
{
    Q_UNUSED(ctx);
    Q_UNUSED(conn);
    Q_UNUSED(user_data);
    messageToSubscriber(CTX_ON_ACCEPT,"Context was accepted", ctx );
    return false;
}

nopoll_bool __ctx_on_open_handler(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data)
{
    Q_UNUSED(ctx);
    Q_UNUSED(conn);
    Q_UNUSED(user_data);
    messageToSubscriber(CTX_ON_OPEN,"Context was opened", ctx);
    return false;
}

nopoll_bool __ctx_on_ready_handler(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data)
{
    Q_UNUSED(ctx);
    Q_UNUSED(conn);
    Q_UNUSED(user_data);
    messageToSubscriber(CTX_ON_READY,"Context is ready", ctx);
    return false;
}

void __report_log_handler (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data)
{
    Q_UNUSED(ctx);
    Q_UNUSED(user_data);
    switch (level) {
    case NOPOLL_LEVEL_CRITICAL:
        messageToSubscriber(GENERAL_LOG_CRITICAL, log_msg, ctx);
        break;
    case NOPOLL_LEVEL_WARNING:
        messageToSubscriber(GENERAL_LOG_WARNING, log_msg, ctx);
        break;
    case NOPOLL_LEVEL_DEBUG:
#ifndef QT_DEBUG
    return;
#endif
        messageToSubscriber(GENERAL_LOG_DEBUG, log_msg, ctx);
        break;
    }
    return;
}

void __on_close_connection_handler (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
    Q_UNUSED(ctx);
    Q_UNUSED(conn);
    Q_UNUSED(user_data);
    messageToSubscriber(CONNECTION_ON_CLOSE, "Connection was closed", ctx);
    return;
}

nopoll_bool __on_ready_connection_handler (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
    Q_UNUSED(ctx);
    Q_UNUSED(conn);
    Q_UNUSED(user_data);
    messageToSubscriber(CONNECTION_ON_READY, "Connection is ready", ctx);
    return true;
}

GraphQlSubscriber::GraphQlSubscriber(GraphQlsubscriberPool *parent) : QObject(parent)
, m_ctx(0)
, m_connection(0)
, m_addr("172.16.254.103")
, m_port(8001)
, m_tries_number(10)
, m_wait_until_connect(10)
{
    m_create_connection_thread = 0;
    m_worker_thread = 0;
    d = parent;
    setId(-1);
}

GraphQlSubscriber::GraphQlSubscriber(const QString &addr, const int &port, GraphQlsubscriberPool *parent) : QObject(parent)
, m_ctx(0)
, m_connection(0)
, m_addr(addr)
, m_port(port)
, m_tries_number(10)
, m_wait_until_connect(10)
{
    d = parent;
    m_create_connection_thread = 0;
    m_worker_thread = 0;
    setId(-1);
}

GraphQlSubscriber::~GraphQlSubscriber()
{
    eraseCtx();
    d = 0;
}

bool GraphQlSubscriber::isBusy()
{
    if(!m_create_connection_thread)
        return false;
    return m_create_connection_thread->isRunning();
}

void GraphQlSubscriber::init()
{
    if(isBusy())
    {
        QTimer::singleShot(3000,this,SLOT(init()));
        return;
    }
    if(m_ctx)
        unsubscribe();

    m_ctx = createCtx();
    if(m_ctx) {
        createConnection(m_addr, m_port);
    }    
}

void GraphQlSubscriber::subscribe()
{
    if(isBusy()) {
        QTimer::singleShot(3000,this,SLOT(subscribe()));
        return;
    }
    const char* mes = getSubscriptionQuery();
    int length = strlen(mes);
    int iRes = nopoll_conn_send_text (m_connection, mes, length) ;
    if (iRes != length) {
        QString err = "ERROR: failed to send message.. Sent: " + QString::number(iRes);
                    __report_log_handler(m_ctx, NOPOLL_LEVEL_DEBUG, err.toLocal8Bit(), 0);
                    unsubscribe();
                    return;
            }
    else
        __report_log_handler(m_ctx, NOPOLL_LEVEL_DEBUG, "Message has sent!", 0);
   if (! nopoll_conn_is_ok (m_connection)) {
           __report_log_handler(m_ctx, NOPOLL_LEVEL_DEBUG, "ERROR: connection failure found after send operation with broken header....", 0);
           unsubscribe();
           return;
   }
   else
       __report_log_handler(m_ctx, NOPOLL_LEVEL_DEBUG, "nopoll_conn_is_ok OK!", 0);
    char tries = m_tries_number;
    noPollMsg      * msg_ref;
    while (tries > 0 ) {
        msg_ref = nopoll_conn_get_msg (m_connection);
        if (msg_ref)
          break;
        nopoll_sleep (SLEEP);
        tries--;
    }
    if (msg_ref == NULL) {
        unsubscribe();
        return;
    }
    const unsigned char * payload = nopoll_msg_get_payload(msg_ref);
    emit signOnGetPayload(QString((char*)payload));
    nopoll_msg_unref (msg_ref);
    m_worker_thread = new WorkerThread(m_connection);
    m_worker_thread->setCtx(m_ctx);
    connect(m_worker_thread, SIGNAL(signData(QByteArray)), this, SLOT(slotData(QByteArray)));
    connect(m_worker_thread, SIGNAL(signError(QByteArray)), this, SLOT(slotError(QByteArray)));
    connect(m_worker_thread, SIGNAL(finished()), this, SLOT(workerThreadFinished()));
    m_worker_thread->start();
    return;
}

void GraphQlSubscriber::unsubscribe()
{
    qDebug() << getId() <<" unsubscribe";
    if(isBusy()) {
        QTimer::singleShot(3000,this,SLOT(unsubscribe()));
        return;
    }
    eraseCtx();
}

void GraphQlSubscriber::setMessage(const MESSAGE_TYPE &type, const char *message)
{
    switch (type) {
    case CTX_ON_ACCEPT:
        //do something
        break;
    case CTX_ON_OPEN:
        //do something
        break;
    case CTX_ON_READY:
        //do something
        break;
    case CONNECTION_ON_CLOSE:
        //do something
        break;
    case CONNECTION_ON_READY:
        //do something
        break;
    case GENERAL_LOG_CRITICAL:
        //do something
        break;
    case GENERAL_LOG_DEBUG:
        //do something
        break;
    case GENERAL_LOG_WARNING:
        //do something
        break;
    default:
        break;
    }
    emit signDebugMessage(QString(message));
}

void GraphQlSubscriber::setSubscriptionQuery(const QString &query)
{
    setPure_query(query);
    QString id = QString::number(getId()) + " " + query;
    m_subscriptionQuery = "{\"id\":\"{597272e6-2c5f-4630-9a21-f7d95b0e2a28}\",\"payload\":{\"namedQuery\":\"\",\"operationName\":\"\",\"query\":\"" + query + "\",\"variables\":null},\"type\":\"start\"}";
    signDebugMessage("full text of new query is: " + m_subscriptionQuery);
}

void GraphQlSubscriber::eraseCtx()
{   
    qDebug() << getId() <<" eraseCtx";
    try {
        if(m_create_connection_thread)
        {
            m_create_connection_thread->blockSignals(true);
            m_create_connection_thread->deleteLater();
            m_create_connection_thread = 0;
        }
        if(m_worker_thread)
        {
            //вызывает deleteLater
            m_worker_thread->deleteConnection();
            m_worker_thread->blockSignals(true);
            m_worker_thread = 0;
        }
        if(m_connection) {
            nopoll_conn_close (m_connection);
            m_connection = 0;
        }
        if(m_ctx) {
            nopoll_ctx_unref (m_ctx);
            m_ctx = 0;
        }
    } catch (...)
    {
        Q_ASSERT(0);
    }

}

noPollCtx *GraphQlSubscriber::createCtx() const
{
    noPollCtx* ctx = nopoll_ctx_new ();
    if (!ctx) {
        __report_log_handler(ctx, NOPOLL_LEVEL_CRITICAL,"Not enough memory for context", 0);
        return 0;
    }
    else
        __report_log_handler(ctx, NOPOLL_LEVEL_DEBUG,"Context created successfully", 0);
    nopoll_log_enable(ctx,1);
    nopoll_log_set_handler(ctx, __report_log_handler, 0);
    int ctx_conns = nopoll_ctx_conns (ctx);
    if (ctx_conns != 0) {
        __report_log_handler(ctx, NOPOLL_LEVEL_DEBUG, "ERROR: There are connections for the new context. Thomething is wrong.", 0);
        nopoll_ctx_unref (ctx);
        return 0;
    }
    return ctx;
}

void GraphQlSubscriber::createConnection(const QString &addr, const int &port)
{
    if(!m_ctx)
        return;
    if(m_create_connection_thread)
        m_create_connection_thread->deleteLater();

    m_create_connection_thread = new WorkerCreateConnectionThread(m_ctx, addr, port, m_postfix);
    connect(m_create_connection_thread, SIGNAL(signError(QByteArray)), this, SLOT(slotError(QByteArray)));
    connect(m_create_connection_thread, SIGNAL(signReady()), this, SLOT(workerCreateConnectionReady()));
    m_create_connection_thread->start();
    emit signDebugMessage("Starting connection thread.. Please wait");
}

int GraphQlSubscriber::getId() const
{
    return m_id;
}

void GraphQlSubscriber::setId(int id)
{
    m_id = id;
}

const char *GraphQlSubscriber::getSubscriptionQuery() const
{
     return m_subscriptionQuery.toUtf8().data();
}

QString GraphQlSubscriber::getPure_query() const
{
    return m_pure_query;
}

void GraphQlSubscriber::setPure_query(const QString &pure_query)
{
    m_pure_query = pure_query;
}

QString GraphQlSubscriber::getPostfix() const
{
    return m_postfix;
}

void GraphQlSubscriber::setPostfix(const QString &postfix)
{
    m_postfix = postfix;
}

int GraphQlSubscriber::getPort() const
{
    return m_port;
}

void GraphQlSubscriber::setPort(int port)
{
    m_port = port;
}

QString GraphQlSubscriber::getAddr() const
{
    return m_addr;
}

void GraphQlSubscriber::setAddr(const QString &addr)
{
    m_addr = addr;
}

void GraphQlSubscriber::slotData(QByteArray ba)
{
    emit signData(ba);
}

void GraphQlSubscriber::slotError(QByteArray ba)
{
    emit signDebugMessage(ba);
}

void GraphQlSubscriber::workerThreadFinished()
{
     emit signDebugMessage("workerThreadFinished");
     WorkerThread *thread = qobject_cast<WorkerThread*>(sender());
     if(thread)
         thread->deleteLater();
}

void GraphQlSubscriber::workerCreateConnectionReady()
{
    if(!m_create_connection_thread)
        return;
    m_connection = m_create_connection_thread->connection();
    if(!m_ctx || !m_connection) {
        emit signDebugMessage("Connection is NOT ready");
        eraseCtx();
        return;
    }
    m_create_connection_thread->deleteLater();
    m_create_connection_thread = 0;
    emit signDebugMessage("Connection is ready");
    emit signConnectionReady();
}

void GraphQlSubscriber::slotPingError(QByteArray ba)
{
    unsubscribe();
    emit signDebugMessage("Ping was failed, unsubscribe executed, need resubscription");
}

WorkerThread::WorkerThread(noPollConn *connection)
{
    Q_ASSERT(connection);
    m_connection = connection;
    m_deletedConnection = false;
}

WorkerThread::~WorkerThread()
{
    qDebug() << "worker thread was killed";
}

void WorkerThread::run()
{
    char buffer[USHRT_MAX];
    while (true) {
        if(m_deletedConnection) {
            this->deleteLater();
            return;
        }
        int iRes = nopoll_conn_read(m_connection, buffer, USHRT_MAX, true, READ_TIME_OUT);
        if (iRes > 0) {
            QString stat = QString::number(iRes) + " bytes of data was received";
            emit signError(stat.toLocal8Bit());
            emit signData(buffer);
            memset (buffer, 0, iRes);
        }
        else if (iRes == 0)
            emit signError("No data (nopoll_conn_read == 0)");
        else
            emit signError("No content is available to be read (nopoll_conn_read < 0");
    }
}

void WorkerThread::setCtx(noPollCtx *newCtx)
{
    m_ctx = newCtx;
}

WorkerCreateConnectionThread::WorkerCreateConnectionThread(noPollCtx *ctx, const QString &addr, const int &port, const QString &postfix)
{
     m_ctx = ctx;
     m_addr = addr;
     m_port = port;
     m_connection = 0;
     m_wait_until_connect = 10;
     m_postfix = postfix;
     m_isStopped = false;
}

void WorkerCreateConnectionThread::run()
{
    QString url = "/" + m_postfix;
    m_connection = nopoll_conn_new (m_ctx,
                                        m_addr.toStdString().c_str(),
                                    QString::number(m_port).toStdString().c_str(),
                                    NULL,
                                    url.toStdString().c_str(),
                                    NULL,
                                    NULL);


    if (!m_ctx || !nopoll_conn_is_ok (m_connection)) {
        emit signError("Can`t establish connection");
        nopoll_conn_close (m_connection);
        m_connection = 0;
        return;
    }
    nopoll_conn_set_on_close (m_connection, __on_close_connection_handler, NULL);
    nopoll_conn_set_on_ready(m_connection, __on_ready_connection_handler, NULL);

    if (!m_ctx || nopoll_ctx_conns (m_ctx) != 1) {
        emit signError("ERROR: There are too much connections for the current session. Thomething is wrong.");
        nopoll_conn_close (m_connection);
        m_connection = 0;
        return;
    }
    if (!m_ctx || !nopoll_conn_wait_until_connection_ready (m_connection, m_wait_until_connect)) {
        emit signError("Connection is NOT ready");
        nopoll_conn_close (m_connection);
        m_connection = 0;
        return;
    }

    emit signReady();
}

noPollConn *WorkerCreateConnectionThread::connection() const
{
    return m_connection;
}

WorkerPingThread::WorkerPingThread()
{
    is_stopped = true;
}

void WorkerPingThread::run()
{
    return;
    QString status = "PING is started";
    emit signStatus(status);
    is_stopped = false;
    char counter = PING_FAIL_COUNT;


    while(!is_stopped)
    {
        sleep(5);
        if(is_stopped)
            return;
        noPollCtx* ctx = nopoll_ctx_new ();
        if (!ctx) {
            return;
        }
        int ctx_conns = nopoll_ctx_conns (ctx);
        if (ctx_conns != 0) {
            nopoll_ctx_unref (ctx);
            return;
        }
        if(is_stopped || !ctx)
        {            
            if(ctx)
                nopoll_ctx_unref (ctx);
            QString status = "PING is stopped";
            emit signStatus(status);
            return;
        }
        noPollConn *conn = nopoll_conn_new (ctx,
                                           m_addr.toStdString().c_str(),
                                       QString::number(m_port).toStdString().c_str(),
                                       NULL,
                                       m_postfix.toStdString().c_str(),
                                       NULL,
                                       NULL);

       if (! nopoll_conn_is_ok (conn) ) {
           counter--;
           QString status = "PING nopoll_conn_is_ok -> fail, decrease counter: " + QString::number(counter) + " of " + QString::number(PING_FAIL_COUNT);
           emit signStatus(status);
       }
       else if (! nopoll_conn_wait_until_connection_ready (conn, WAIT_UNTIL_CONNECT)) {
           counter--;
           QString status = "PING nopoll_conn_wait_until_connection_ready -> fail, decrease counter: " + QString::number(counter) + " of " + QString::number(PING_FAIL_COUNT) ;
           emit signStatus(status);
       }
       else if(counter < PING_FAIL_COUNT)
       {
           counter++;
           QString status = "PING is OK, increase counter: " + QString::number(counter) + " of " + QString::number(PING_FAIL_COUNT) ;
           emit signStatus(status);
       }

       if(counter == 0)
           emit signError("NO PING");
       nopoll_conn_close (conn);
       conn = 0;
       nopoll_ctx_unref (ctx);
       ctx = 0;
    }
}

void WorkerPingThread::stop()
{
    is_stopped = true;
}

void WorkerPingThread::setAddr(const QString &addr)
{
    m_addr = addr;
}

void WorkerPingThread::setPostfix(const QString &postfix)
{
    m_postfix = postfix;
}

void WorkerPingThread::setPort(int port)
{
    m_port = port;
}
