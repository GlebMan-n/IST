#ifndef GRAPHQLSUBSCRIBER_H
#define GRAPHQLSUBSCRIBER_H

#include <QObject>
#include <QThread>
#include "nopoll.h"

class GraphQlsubscriberPool;

enum MESSAGE_TYPE {CTX_ON_ACCEPT, CTX_ON_OPEN, CTX_ON_READY, CONNECTION_ON_CLOSE, CONNECTION_ON_READY, GENERAL_LOG_CRITICAL, GENERAL_LOG_DEBUG, GENERAL_LOG_WARNING };

class WorkerPingThread : public QThread
{
    Q_OBJECT
public:
    WorkerPingThread();
    void run();
    void stop();
    void setAddr(const QString &addr);

    void setPostfix(const QString &postfix);

    void setPort(int port);

private:
    bool is_stopped;
    QString         m_addr;
    QString         m_postfix;
    int             m_port;
signals:
    void signStatus(const QString& status);
    void signError(QByteArray ba);
};

class WorkerThread : public QThread
{
    Q_OBJECT
public:
    WorkerThread(noPollConn* connection);
    ~WorkerThread();
    void run();
    void deleteConnection() {m_deletedConnection = true;}
    void setCtx(noPollCtx *newCtx);

private:
    noPollCtx       *m_ctx;
    noPollConn      *m_connection;
    bool            m_deletedConnection;
signals:
    void signData(QByteArray ba);
    void signError(QByteArray ba);
};

class WorkerCreateConnectionThread : public QThread
{
    Q_OBJECT
public:
    WorkerCreateConnectionThread(noPollCtx* ctx, const QString &addr, const int &port, const QString &postfix);
    void run();
    noPollConn *connection() const;
    void stop();
private:
    bool            m_isStopped;
    noPollCtx       *m_ctx;
    noPollConn      *m_connection;
    QString         m_addr;
    QString         m_postfix;
    int             m_port;
    int             m_wait_until_connect;
signals:
    void signReady();
    void signError(QByteArray ba);
};

class GraphQlSubscriber : public QObject
{
    Q_OBJECT
public:

    GraphQlSubscriber(GraphQlsubscriberPool *parent);
    GraphQlSubscriber(const QString &addr, const int &port, GraphQlsubscriberPool *parent = 0);
    ~GraphQlSubscriber();
    bool isBusy();
    void setMessage(const MESSAGE_TYPE &type, const char* message);
    void setSubscriptionQuery(const QString &query);
    QString getAddr() const;
    void setAddr(const QString &addr);
    int getPort() const;
    void setPort(int port);
    QString getPostfix() const;
    void setPostfix(const QString &postfix);
    QString getPure_query() const;
    void setPure_query(const QString &pure_query);
    int getId() const;
    void setId(int id);
    void* getContext() const { return m_ctx; }
private:

    const char* getSubscriptionQuery() const;
    void        eraseCtx();
    noPollCtx   *createCtx() const;
    void        createConnection(const QString& addr, const int& port);
private:
    noPollCtx       *m_ctx;
    noPollConn      *m_connection;
    QString         m_addr;
    QString         m_postfix;
    int             m_port;
    const int       m_tries_number;
    const int       m_wait_until_connect;
    WorkerThread    *m_worker_thread;
    WorkerCreateConnectionThread *m_create_connection_thread;
    QString         m_subscriptionQuery;
    QString         m_payload;    
    QString         m_pure_query;
    int             m_id;
public slots:
    void init();
    void unsubscribe();
    void subscribe();
private slots:    
    void slotData(QByteArray ba);
    void slotError(QByteArray ba);
    void workerThreadFinished();
    void workerCreateConnectionReady();
    void slotPingError(QByteArray ba);
signals:
    void signData(const QByteArray &ba);
    void signDebugMessage(const QString& message);
    void signOnGetPayload(const QString& payload);
    void signConnectionReady();
    void signConnectionClosed(int id);
};


#endif // GRAPHQLSUBSCRIBER_H
