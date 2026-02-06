#ifndef HTTPSENDDATA_H
#define HTTPSENDDATA_H

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QList>

class HttpSendData {
public:
    HttpSendData();
    
    QString protocol() const { return m_protocol; }
    void setProtocol(const QString& protocol) { m_protocol = protocol; }
    
    QString content() const { return m_content; }
    void setContent(const QString& content) { m_content = content; }
    
    QString tag() const { return m_tag; }
    void setTag(const QString& tag) { m_tag = tag; }
    
    QJsonObject toJson() const;
    
    static HttpSendData createMetricData(const QString& uniqueId, 
                                         const QList<QJsonObject>& datas,
                                         const QByteArray& boxPrivateKey,
                                         const QByteArray& servicePublicKey,
                                         const QString& tag);
    
    static HttpSendData createWaterBoxData(const QString& uniqueId,
                                           const QJsonObject& data,
                                           const QByteArray& boxPrivateKey,
                                           const QByteArray& servicePublicKey,
                                           const QString& tag,
                                           bool isPersistent = true);
    
    static HttpSendData createCommand(const QString& uniqueId,
                                      const QString& cmd,
                                      const QString& content,
                                      const QByteArray& boxPrivateKey,
                                      const QByteArray& servicePublicKey,
                                      const QJsonObject& exData = QJsonObject());
    
private:
    static QString encryptData(const QString& data,
                              const QByteArray& boxPrivateKey,
                              const QByteArray& servicePublicKey);
    
    static QString formatNumber(double value, int digits = 3);
    
    QString m_protocol;
    QString m_content;
    QString m_tag;
};

#endif
