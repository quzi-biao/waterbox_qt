#include "HttpSendData.h"
#include "common/AESUtils.h"
#include "common/RSAUtils.h"
#include "common/NetworkUtils.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <zlib.h>

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

// GZIP压缩函数（与DataSender中的相同）
static QByteArray gzipCompressData(const QByteArray& data) {
    if (data.isEmpty()) {
        return QByteArray();
    }
    
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return QByteArray();
    }
    
    stream.avail_in = data.size();
    stream.next_in = (Bytef*)data.data();
    
    QByteArray compressed;
    compressed.resize(data.size() + 1024);
    
    stream.avail_out = compressed.size();
    stream.next_out = (Bytef*)compressed.data();
    
    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&stream);
        return QByteArray();
    }
    
    compressed.resize(stream.total_out);
    deflateEnd(&stream);
    
    return compressed;
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
    
    // 1. JSONObject.toJSONString(datas)
    QJsonArray jsonArray;
    for (const QJsonObject& obj : datas) {
        jsonArray.append(obj);
    }
    QJsonDocument doc(jsonArray);
    QString jsonStr = doc.toJson(QJsonDocument::Compact);
    
    // 2. CompressUtils.gzipCompress(str)
    QByteArray compressed = gzipCompressData(jsonStr.toUtf8());
    
    // 3. Base64Encoder.encode(data)
    QString encoded = QString::fromLatin1(compressed.toBase64());
    
    // 4. JSONObject.toJSONString(encode) - 将字符串序列化为JSON字符串
    QJsonDocument encodeDoc(QJsonArray{encoded});
    QString encodeJsonStr = encodeDoc.toJson(QJsonDocument::Compact);
    // 去掉数组的 [ ]
    encodeJsonStr = encodeJsonStr.mid(1, encodeJsonStr.length() - 2);
    
    // 5. uniqueId + ":" + jsonString
    result.setContent(uniqueId + ":" + encodeJsonStr);
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
