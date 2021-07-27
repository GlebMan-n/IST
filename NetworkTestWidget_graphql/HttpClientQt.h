#ifndef HTTPCLIENTQT_H
#define HTTPCLIENTQT_H

#include <QObject>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QXmlStreamReader>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QMap>
#include <QMapIterator>
#include <QNetworkRequest>
#include <QNetworkCacheMetaData>
#include <QElapsedTimer>

class HttpClientQt : public QObject
{
    Q_OBJECT

public:
    HttpClientQt(QObject *parent);
    ~HttpClientQt();
	QString m_lastError;
	qint64 m_lastOperationTime;
	QNetworkAccessManager* m_networkAccessManager;
    QVariant getData() const
    {
        return m_Data;
    }
    void setData(QVariant val)
    {
        m_Data = val;
    }
    void sendData(const QString &url, const QMap<QString,QString> &rawHeaders, const QByteArray &ba = QByteArray());
    void sendData(const QNetworkRequest &request, const QByteArray &ba);
    void sendData(const QNetworkRequest &request);
	QString getLastError() const { return m_lastError; }
	qint64 getLastOperationTime() const { return m_lastOperationTime; }
private:
    QVariant m_Data;
	QElapsedTimer m_eTimer;	
protected:
    void toLog(const QString &log);
    void toLog(const QByteArray &log);
private:
    void logRequest(const QNetworkRequest &request,const QString &body = QString());
private slots:
    void finished(QNetworkReply* reply);
    void slotDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void slotUploadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void finishedSendData();
    void downloadReadyRead();
    void slotBytesWritten(qint64 bytes);
    void slotReadyRead();
signals:
    void signDebug(const QString &log);
    void signResult(const QString &qStrResult);
    void signData(const QByteArray &ba);
    void signFinished(const int &iRes);
    void signUploadProg(const qint64 &bytesReceived, const qint64 &bytesTotal);
    void signDownloadProg(const qint64 &bytesReceived, const qint64 &bytesTotal);
    void signFinished();

};

#endif // HTTPCLIENTQT_H
