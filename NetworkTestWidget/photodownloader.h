#ifndef PHOTODOWNLOADER_H
#define PHOTODOWNLOADER_H

#include <QObject>

#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QtDebug>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkCookieJar>
#include <QNetworkCookie>

class PhotoDownloader : public QObject
{
    Q_OBJECT
public:
    PhotoDownloader(const QString &uid, const QString &url, const QString &krbServiceName, QObject* parent = 0);
    void init();
    void init(QMap<QString,QString> raw_headers);
private:
    QNetworkAccessManager m_manager;
    QNetworkRequest m_request;
    QString         m_uid;
    QString         m_url;
    QString         m_krbServiceName;
    QByteArray      m_data;
private slots:
    void slot_authenticationRequired(QNetworkReply*,QAuthenticator*);
    void slot_proxyAuthenticationRequired(const QNetworkProxy&,QAuthenticator*);
    void slot_finished_first();
    void slot_finished_second();
    void slot_finished_third();
signals:
    void signDownloadFinished(QByteArray ba);
};

#endif // PHOTODOWNLOADER_H
