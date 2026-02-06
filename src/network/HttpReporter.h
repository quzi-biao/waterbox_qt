#ifndef HTTPREPORTER_H
#define HTTPREPORTER_H

#include "interfaces/IDataReporter.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class HttpReporter : public QObject, public IDataReporter {
    Q_OBJECT
    
public:
    explicit HttpReporter(QObject* parent = nullptr);
    ~HttpReporter();
    
    bool establishConnection(const QString& url) override;
    bool reportData(const QJsonObject& data) override;
    void closeConnection() override;
    bool isConnected() const override;
    
    QString getLastResponse() const override;
    
signals:
    void responseReceived(const QString& response);
    void reportSuccess();
    void reportFailed(const QString& error);
    
private slots:
    void onReplyFinished(QNetworkReply* reply);
    
private:
    QNetworkAccessManager* m_manager;
    QString m_url;
    QString m_lastResponse;
    bool m_connected;
};

#endif
