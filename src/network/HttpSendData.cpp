#include "HttpSendData.h"
#include "common/AESUtils.h"
#include "common/RSAUtils.h"
#include "common/NetworkUtils.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

HttpSendData::HttpSendData() {
}

QJsonObject HttpSendData::toJson() const {
    QJsonObject obj;
    obj["protocol"] = m_protocol;
    obj["content"] = m_content;
    if (!m_tag.isEmpty()) {
        obj["tag"] = m_tag;
    }
    return obj;
}

HttpSendData HttpSendData::createMetricData(const QString& uniqueId,
                                            const QList<QJsonObject>& datas,
                                            const QByteArray& boxPrivateKey,
                                            const QByteArray& servicePublicKey,
                                            const QString& tag) {
    HttpSendData result;
    
    if (datas.isEmpty()) {
        return result;
    }
    
    QJsonArray jsonArray;
    for (const QJsonObject& obj : datas) {
        jsonArray.append(obj);
    }
    
    QJsonDocument doc(jsonArray);
    QString jsonStr = doc.toJson(QJsonDocument::Compact);
    
    QByteArray compressed = qCompress(jsonStr.toUtf8(), 9);
    QString encoded = compressed.toBase64();
    
    result.setContent(uniqueId + ":" + QJsonDocument(QJsonArray{encoded}).toJson(QJsonDocument::Compact));
    result.setProtocol("pumpMetrics");
    result.setTag(tag);
    
    return result;
}

HttpSendData HttpSendData::createWaterBoxData(const QString& uniqueId,
                                              const QJsonObject& data,
                                              const QByteArray& boxPrivateKey,
                                              const QByteArray& servicePublicKey,
                                              const QString& tag,
                                              bool isPersistent) {
    HttpSendData result;
    
    QString content;
    if (!isPersistent) {
        content = "NP:";
    }
    
    content += QString::number(data.value("timestamp").toVariant().toLongLong()) + ",";
    content += formatNumber(data.value("hlimit").toDouble()) + ",";
    content += formatNumber(data.value("llimit").toDouble()) + ",";
    content += formatNumber(data.value("climit").toDouble()) + ",";
    content += formatNumber(data.value("wh").toDouble()) + ",";
    content += formatNumber(data.value("wflow").toDouble()) + ",";
    content += formatNumber(data.value("valveOpening").toDouble()) + ",";
    content += formatNumber(data.value("cl0").toDouble()) + ",";
    content += formatNumber(data.value("cl1").toDouble()) + ",";
    content += formatNumber(data.value("temperature").toDouble()) + ",";
    content += formatNumber(data.value("ph").toDouble()) + ",";
    content += formatNumber(data.value("turbidity").toDouble()) + ",";
    content += formatNumber(data.value("toc").toDouble()) + ",";
    content += "N,";
    content += formatNumber(data.value("waterAge").toDouble()) + ",";
    content += formatNumber(data.value("press").toDouble()) + ",";
    content += "N,N,";
    content += formatNumber(data.value("cumulativeFlow").toDouble()) + ",";
    content += formatNumber(data.value("cumulativePower").toDouble()) + ",";
    content += formatNumber(data.value("clSafe").toDouble());
    
    QString encrypted = encryptData(content, boxPrivateKey, servicePublicKey);
    
    result.setContent(uniqueId + ":" + encrypted);
    result.setProtocol("waterbox");
    result.setTag(tag);
    
    return result;
}

HttpSendData HttpSendData::createCommand(const QString& uniqueId,
                                        const QString& cmd,
                                        const QString& content,
                                        const QByteArray& boxPrivateKey,
                                        const QByteArray& servicePublicKey,
                                        const QJsonObject& exData) {
    HttpSendData result;
    
    QJsonObject contentMap;
    contentMap["cmd"] = cmd;
    contentMap["exData"] = exData;
    contentMap["content"] = encryptData(content, boxPrivateKey, servicePublicKey);
    contentMap["uniqueId"] = uniqueId;
    
    result.setContent(QJsonDocument(contentMap).toJson(QJsonDocument::Compact));
    result.setProtocol("waterbox_cmd");
    
    return result;
}

QString HttpSendData::encryptData(const QString& data,
                                  const QByteArray& boxPrivateKey,
                                  const QByteArray& servicePublicKey) {
    QString password = NetworkUtils::generateUUID().replace("-", "");
    
    QString encryptedData = AESUtils::encrypt(password, data);
    
    QByteArray encryptedPwd = RSAUtils::encryptByPublicKey(password.toUtf8(), servicePublicKey);
    
    return encryptedPwd.toBase64() + "," + encryptedData;
}

QString HttpSendData::formatNumber(double value, int digits) {
    if (qIsNaN(value) || qIsInf(value)) {
        return "N";
    }
    return QString::number(value, 'f', digits);
}
