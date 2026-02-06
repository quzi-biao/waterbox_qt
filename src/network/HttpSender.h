#ifndef HTTPSENDER_H
#define HTTPSENDER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QList>

class DatabaseManager;

class HttpSender : public QObject {
    Q_OBJECT
    
public:
    explicit HttpSender(QObject* parent = nullptr);
    ~HttpSender();
    
    void initialize();
    
    void sendMetrics(const QList<QJsonObject>& datas);
    
    void sendWaterBoxData(const QJsonObject& data, bool isPersistent = true);
    
    void sendCommand(const QString& cmd, const QString& content, const QJsonObject& exData = QJsonObject());
    
    void sendRawData(const QJsonObject& httpSendData);
    
    QString uniqueId() const { return m_uniqueId; }
    
    bool isInitialized() const { return m_initialized; }
    
signals:
    void publicKeyReceived();
    void sendSuccess();
    void sendFailed(const QString& error);
    void responseReceived(const QByteArray& data);
    
private slots:
    void onPublicKeyReplyFinished();
    void onDataReplyFinished();
    
private:
    void getBoxRsaKeys();
    void getServicePublicKey();
    QString getUniqueId();
    QString getStaticString();
    
    QNetworkAccessManager* m_networkManager;
    
    QByteArray m_servicePublicKey;
    QByteArray m_boxPublicKey;
    QByteArray m_boxPrivateKey;
    
    QString m_uniqueId;
    QString m_metricUpUrl;
    QString m_publicKeyUrl;
    
    bool m_initialized;
    bool m_ignore;
    
    static const QString BOX_PRIVATE_KEY;
    static const QString BOX_PUBLIC_KEY;
    static const QString BOX_ADDRESS;
    static const QString STATIC_STRING;
    static const QString PUBLICKEY_RESOURCE;
    static const QString METRIC_RESOURCE;
};

#endif
