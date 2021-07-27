#include "graphqlsubscriberpool.h"
#include "graphqlsubscriber.h"
#include <QTimer>

GraphQlsubscriberPool::GraphQlsubscriberPool(const settings_graphql_subscription_service &settings, QObject *parent) : QObject(parent)
  , m_count(0)
  , m_settings(settings)
{

}

GraphQlsubscriberPool::~GraphQlsubscriberPool()
{
}

int GraphQlsubscriberPool::addSubscription(const QString &query)
{
    GraphQlSubscriber *subscriber = new GraphQlSubscriber(this);

    connect(subscriber, SIGNAL(signData(QByteArray)),
            this,SLOT(slotData(QByteArray)));
    connect(subscriber, SIGNAL(signDebugMessage(QString)),
            this,SLOT(slotDebugMessage(QString)));
    connect(subscriber, SIGNAL(signOnGetPayload(QString)),
            this,SLOT(slotOnGetPayload(QString)));
    connect(subscriber, SIGNAL(signConnectionReady()),
            this,SLOT(slotOnConnectionReady()));

    if (!subscriber)
        return -1;

    subscriber->setAddr(m_settings.m_ip);
    subscriber->setPort(m_settings.m_port);
    subscriber->setPostfix(m_settings.m_endpoint);
    subscriber->setId(m_count++);
    subscriber->setSubscriptionQuery(query);    
    m_map_subcriptions.insert(subscriber->getId(),subscriber);
    subscriber->init();    
    return subscriber->getId();
}

QMap<int, GraphQlSubscriber*> GraphQlsubscriberPool::getSubscriptions()
{
    return m_map_subcriptions;
}

QString GraphQlsubscriberPool::getQueryById(int id)
{
    return m_map_subcriptions[id]->getPure_query();
}

bool GraphQlsubscriberPool::isSubscriptionExists(int id)
{
    return m_map_subcriptions.contains(id);
}

void GraphQlsubscriberPool::askToRemoveSubscription(int id)
{
    if(!m_map_subcriptions.contains(id))
        return;
    removeSubscription(m_map_subcriptions.value(id));
}

void GraphQlsubscriberPool::askToSubscribe(int id)
{
    if(!m_map_subcriptions.contains(id))
        return;
    if(m_map_subcriptions[id])
        m_map_subcriptions[id]->subscribe();
}

void GraphQlsubscriberPool::slotData(QByteArray ba)
{
    GraphQlSubscriber* subscriber = qobject_cast<GraphQlSubscriber*>(sender());
    if(!subscriber)
        return;
    int id = subscriber->getId();
    emit signSubscriptionData(id,ba);
}

void GraphQlsubscriberPool::slotDebugMessage(QString message)
{
    GraphQlSubscriber* subscriber = qobject_cast<GraphQlSubscriber*>(sender());
    if(!subscriber)
        return;
    int id = subscriber->getId();
    emit signSubscriptionMessage(id,message);
}

void GraphQlsubscriberPool::slotOnGetPayload(QString payload)
{
    GraphQlSubscriber* subscriber = qobject_cast<GraphQlSubscriber*>(sender());
    if(!subscriber)
        return;
    int id = subscriber->getId();
    emit signSubscriptionPayload(id,payload);
}

void GraphQlsubscriberPool::slotOnConnectionReady()
{
    GraphQlSubscriber* subscriber = qobject_cast<GraphQlSubscriber*>(sender());
    if(!subscriber)
        return;
    int id = subscriber->getId();
    m_map_contexts.insert(id,(void*)subscriber->getContext());
    emit signSubscriptionReady(id);
}

void GraphQlsubscriberPool::removeAllSubscribes()
{
    const QList<GraphQlSubscriber*> list = m_map_subcriptions.values();
    for (unsigned i = 0; i < (unsigned) list.length(); i++)
        removeSubscription(list[i]);
}

GraphQlSubscriber *GraphQlsubscriberPool::getSubscriberByCondext(void *ctx)
{
    const QList<GraphQlSubscriber*> &list = m_map_subcriptions.values();
    for (unsigned i = 0; i < (unsigned) list.length(); i++)
        if (list.at(i) && list.at(i)->getContext() == ctx)
            return list.at(i);
    return 0;
}

GraphQlSubscriber *GraphQlsubscriberPool::getSubscribe(int id)
{
    return m_map_subcriptions.value(id);
}

void GraphQlsubscriberPool::removeSubscription(GraphQlSubscriber *subscription)
{    
    if(subscription) {
        int i = subscription->getId();
        m_map_subcriptions.remove(i);
        subscription->unsubscribe();
    }
}

QMap<int, void *> GraphQlsubscriberPool::getMap_contexts() const
{
    return m_map_contexts;
}

void GraphQlsubscriberPool::on_connection_closed(void *context)
{
    int key = m_map_contexts.key(context);
    emit signSubscriptionClose(key);
    m_map_contexts.remove(key);
}
