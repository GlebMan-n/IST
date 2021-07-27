#ifndef GRAPHQLSUBSCRIBERPOOL_H
#define GRAPHQLSUBSCRIBERPOOL_H

#include <QObject>
#include <QMap>

class GraphQlSubscriber;

/*
Структура настроек для подключения к сервису, передаем в конструктор пула
*/
struct settings_graphql_subscription_service
{
    QString m_ip;
    int m_port;
    QString m_endpoint;
};

/*
Класс управления подписками на события в GraphQL.
Задумка: добавляем запрос к сервису, далее пул должен поддерживать подключение.
Сообщать необходимый минимум информации наружу через сигнал/слотные соединения.
Работать с подписками можно по их номерам, которые возвращаются при добавлении запроса.
При первоначальной инициализации пула, передаем настройки для подключению к единствунному
сервису, при необходимоста подключения к другому сервису нужно будет создать другой экземпляр
класса пула
*/

class GraphQlsubscriberPool : public QObject
{
    Q_OBJECT
public:
    GraphQlsubscriberPool(const settings_graphql_subscription_service& settings, QObject *parent);
    ~GraphQlsubscriberPool();
    //подписываемся на запрос, получаем номер подписки
    int addSubscription(const QString& query);
    //возвращает все подписки и запросы их характерезующие
    QMap<int,GraphQlSubscriber*> getSubscriptions();
    //возвращает запрос по идентификатору подписки
    QString getQueryById(int id);
    //проверка существует ли подписка с идентификатором в пуле
    bool isSubscriptionExists(int id);
    //запросить удаление подписки по идентификатору
    void askToRemoveSubscription(int id);
    //удалить все подписки
    void removeAllSubscribes();
    GraphQlSubscriber* getSubscriberByCondext(void *ctx);
    QMap<int, void *> getMap_contexts() const;
    void on_connection_closed(void*);
public slots:
    //запросить подписку
    void askToSubscribe(int id);
    //слот принимает данные от подписки
    void slotData(QByteArray ba);
    //слот принимает сводную информацию о работе классов подписок
    void slotDebugMessage(QString message);
    //слот принимает возвращаемые данные при инициализации подписки
    void slotOnGetPayload(QString payload);
    //слот срабатывает, когда подписка готова к работе
    void slotOnConnectionReady();
signals:
    //слот срабатывает по удалению подписки
    void signSubscriptionRemoved(int id);
    //слот срабатывает по запуску подписки
    void signSubscriptionStarted(int id);
    //сигнал передает данные от подписки с идентификатором
    void signSubscriptionData(int id, QByteArray ba);
    //сигнал передает сводную информацию о работе классов подписок
    void signSubscriptionMessage(int id, QString message);
    //сигнал принимает возвращаемые данные при инициализации подписки
    void signSubscriptionPayload(int id, QString payload);
    //сигнал срабатывает, когда подписка готова к работе
    void signSubscriptionReady(int id);
    //сигнал срабатывает, когда подписка закрыта (можно сработать внезапно)
    void signSubscriptionClose(int id);
private:
    GraphQlSubscriber* getSubscribe(int id);
    void removeSubscription(GraphQlSubscriber* subscription);
private:
    QMap<int,GraphQlSubscriber*>            m_map_subcriptions;
    QMap<int,void*>                         m_map_contexts;
    int                                     m_count;
    settings_graphql_subscription_service   m_settings;
};

#endif // GRAPHQLSUBSCRIBERPOOL_H
