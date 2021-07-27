#include "photodownloader.h"
#include "kerbticket.h"

#define AUTH_HEADER "WWW-Authenticate"
#define AUTH_REQ_HEADER "Authorization"
#define AUTH_REQ_NEGOTIATE "Negotiate"

PhotoDownloader::PhotoDownloader(const QString &uid, const QString &url, const QString &krbServiceName, QObject* parent) : QObject(parent)
    , m_uid(uid)
    , m_url(url)
    , m_krbServiceName(krbServiceName)
{
    connect (&m_manager,
             SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
             this,
             SLOT(slot_authenticationRequired(QNetworkReply*,QAuthenticator*)));

    connect (&m_manager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&,QAuthenticator*)),
             this, SLOT(slot_proxyAuthenticationRequired(QNetworkProxy*,QAuthenticator*)));
}

void PhotoDownloader::init()
{
    QMap<QString,QString> raw_headers;
    init(raw_headers);
}

void PhotoDownloader::init(QMap<QString,QString> raw_headers)
{
    qDebug() << "init\n";
    QMap<QString,QString> raw_headers_;
    if(raw_headers_.isEmpty())
    {
        //класс может быть создан много раз, заголовки одинаковы
        //нет смысла их пересоздавать в случае если их подготовили
        //в классе уровнем выше
        raw_headers_["Host"] = "uzel.first.int";
        raw_headers_["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64; rv:72.0) Gecko/20100101 Firefox/72.0";
        raw_headers_["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8";
        raw_headers_["Accept-Language"] = "ru-RU,ru;q=0.8,en-US;q=0.5,en;q=0.3";
        raw_headers_["Accept-Encoding"] = "gzip, deflate";
        raw_headers_["Connection"] = "keep-alive";
        raw_headers_["Upgrade-Insecure-Requests"] = "1";
        raw_headers_["Pragma"] = "no-cache";
        raw_headers_["Cache-Control"] = "no-cache";
    }
    QMap<QString, QString>::const_iterator i = raw_headers_.constBegin();
    while (i != raw_headers_.constEnd())
    {
        m_request.setRawHeader(i.key().toUtf8(),i.value().toUtf8());
        ++i;
    }
    QString url_whole = m_url +m_uid;
    m_request.setUrl(QUrl(url_whole));

    QNetworkReply* networkReply = m_manager.get(m_request);
    connect(networkReply,   SIGNAL(finished()),
            this,           SLOT(slot_finished_first()));
}

void PhotoDownloader::slot_authenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
    qDebug() << "AUTH REQUIRED " << reply << auth;
}

void PhotoDownloader::slot_proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth)
{
    Q_UNUSED(proxy)
    qDebug() << "PROXY AUTH REQUIRED "<< auth;
}

void PhotoDownloader::slot_finished_first()
{
    qDebug() << "\n slot_finished_first() \n\n";
    QNetworkReply* networkReply = qobject_cast<QNetworkReply*> (sender());
    if(!networkReply)
        return;
    qDebug() << AUTH_HEADER << " : " << networkReply->rawHeader(AUTH_HEADER);
    if(networkReply->rawHeader(AUTH_HEADER).contains(AUTH_REQ_NEGOTIATE))
    {
        qDebug() << networkReply->rawHeaderList();
        qDebug() << networkReply->rawHeader("set-cookie");

        QNetworkRequest request2 = m_request;

        //заполняет m_token
        KerbTicket krbTicket(m_krbServiceName);
        request2.setRawHeader(QString(AUTH_REQ_HEADER).toLocal8Bit() , QByteArray(AUTH_REQ_NEGOTIATE) + " " + krbTicket.getKrbTicket().toBase64());
        QNetworkReply* networkReply = m_manager.get(request2);
        connect(networkReply, SIGNAL(finished()),
                this,         SLOT(slot_finished_second()));
    }
    networkReply->deleteLater();
}

void PhotoDownloader::slot_finished_second()
{
    qDebug() << "\n slot_finished_second() \n\n";
    QNetworkReply* networkReplySender = qobject_cast<QNetworkReply*> (sender());
    if(!networkReplySender)
        return;
    qDebug() << networkReplySender->rawHeaderList();
    qDebug() << networkReplySender->rawHeader("set-cookie");
    m_data = networkReplySender->readAll();

    QNetworkReply* networkReply = m_manager.get(m_request);
    connect(networkReply, SIGNAL(finished()),
            this,         SLOT(slot_finished_third()));

    networkReplySender->deleteLater();
}

void PhotoDownloader::slot_finished_third()
{
    qDebug() << "\n slot_finished_third() \n\n";
    QNetworkReply* networkReplySender = qobject_cast<QNetworkReply*> (sender());
    if(!networkReplySender)
        return;
    qDebug() << networkReplySender->rawHeaderList();
    qDebug() << networkReplySender->rawHeader("set-cookie");
    qDebug() << networkReplySender->readAll();
    networkReplySender->deleteLater();
    emit signDownloadFinished(m_data);
}
