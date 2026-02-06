#include "HttpSender.h"
#include "HttpSendData.h"
#include "core/ConfigManager.h"
#include "database/DatabaseManager.h"
#include "common/RSAUtils.h"
#include "common/NetworkUtils.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>
#include <QDebug>

const QString HttpSender::BOX_PRIVATE_KEY = "boxPrivateKey";
const QString HttpSender::BOX_PUBLIC_KEY = "boxPublicKey";
const QString HttpSender::BOX_ADDRESS = "boxAddress";
const QString HttpSender::STATIC_STRING = "staticString";
const QString HttpSender::PUBLICKEY_RESOURCE = "/metric/publickey";
const QString HttpSender::METRIC_RESOURCE = "/metric/up";

HttpSender::HttpSender(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_initialized(false),
      m_ignore(false) {
}

HttpSender::~HttpSender() {
}

void HttpSender::initialize() {
    m_uniqueId = getUniqueId();
    getBoxRsaKeys();
    
    QString serviceAddress = ConfigManager::instance()->get("serviceAddress", "").toString();
    if (serviceAddress.isEmpty()) {
        serviceAddress = ConfigManager::instance()->get("service_address", "").toString();
    }
    
    if (serviceAddress.isEmpty()) {
        m_ignore = true;
        qWarning() << "Service address not configured, HttpSender disabled";
        return;
    }
    
    if (!serviceAddress.startsWith("http://") && !serviceAddress.startsWith("https://")) {
        serviceAddress = "http://" + serviceAddress;
    }
    
    if (serviceAddress.endsWith("/")) {
        serviceAddress = serviceAddress.left(serviceAddress.length() - 1);
    }
    
    m_metricUpUrl = serviceAddress + METRIC_RESOURCE;
    m_publicKeyUrl = serviceAddress + PUBLICKEY_RESOURCE;
    
    getServicePublicKey();
}

void HttpSender::sendMetrics(const QList<QJsonObject>& datas) {
    if (m_ignore || !m_initialized) {
        return;
    }
    
    if (datas.isEmpty()) {
        return;
    }
    
    if (m_servicePublicKey.isEmpty()) {
        getServicePublicKey();
        return;
    }
    
    QString tag = ConfigManager::instance()->get("tag", "").toString();
    HttpSendData httpData = HttpSendData::createMetricData(
        m_uniqueId, datas, m_boxPrivateKey, m_servicePublicKey, tag);
    
    QJsonDocument doc(httpData.toJson());
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    
    qInfo() << "发送泵组监控数据:" << httpData.content();
    
    QNetworkRequest request(m_metricUpUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = m_networkManager->post(request, postData);
    connect(reply, &QNetworkReply::finished, this, &HttpSender::onDataReplyFinished);
}

void HttpSender::sendWaterBoxData(const QJsonObject& data, bool isPersistent) {
    if (m_ignore || !m_initialized) {
        return;
    }
    
    if (m_servicePublicKey.isEmpty()) {
        getServicePublicKey();
        return;
    }
    
    QString tag = ConfigManager::instance()->get("tag", "").toString();
    HttpSendData httpData = HttpSendData::createWaterBoxData(
        m_uniqueId, data, m_boxPrivateKey, m_servicePublicKey, tag, isPersistent);
    
    QJsonDocument doc(httpData.toJson());
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    
    qInfo() << "发送水箱数据:" << httpData.content();
    
    QNetworkRequest request(m_metricUpUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = m_networkManager->post(request, postData);
    connect(reply, &QNetworkReply::finished, this, &HttpSender::onDataReplyFinished);
}

void HttpSender::sendCommand(const QString& cmd, const QString& content, const QJsonObject& exData) {
    if (m_ignore || !m_initialized) {
        return;
    }
    
    if (m_servicePublicKey.isEmpty()) {
        getServicePublicKey();
        return;
    }
    
    HttpSendData httpData = HttpSendData::createCommand(
        m_uniqueId, cmd, content, m_boxPrivateKey, m_servicePublicKey, exData);
    
    QJsonDocument doc(httpData.toJson());
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    
    qInfo() << "发送命令:" << cmd << "内容:" << httpData.content();
    
    QNetworkRequest request(m_metricUpUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = m_networkManager->post(request, postData);
    connect(reply, &QNetworkReply::finished, this, &HttpSender::onDataReplyFinished);
}

void HttpSender::sendRawData(const QJsonObject& httpSendData) {
    if (m_ignore || !m_initialized) {
        return;
    }
    
    QJsonDocument doc(httpSendData);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    
    QString protocol = httpSendData.value("protocol").toString();
    qInfo() << "发送数据 - Protocol:" << protocol << "URL:" << m_metricUpUrl;
    qDebug() << "Post data size:" << postData.size() << "bytes";
    
    QNetworkRequest request(m_metricUpUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = m_networkManager->post(request, postData);
    
    // 添加lambda来捕获响应
    connect(reply, &QNetworkReply::finished, this, [this, reply, protocol]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qInfo() << "发送成功 - Protocol:" << protocol << "Response:" << response;
            emit responseReceived(response);
        } else {
            qWarning() << "发送失败 - Protocol:" << protocol 
                      << "Error:" << reply->errorString()
                      << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        }
        reply->deleteLater();
    });
}

void HttpSender::getServicePublicKey() {
    if (m_publicKeyUrl.isEmpty()) {
        return;
    }
    
    qInfo() << "获取服务端公钥:" << m_publicKeyUrl;
    
    QJsonObject params;
    params["uniqueId"] = m_uniqueId;
    params["clientPublicKey"] = QString::fromUtf8(m_boxPublicKey.toBase64());
    params["type"] = "waterbox";
    
    QJsonDocument doc(params);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);
    
    QNetworkRequest request(m_publicKeyUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = m_networkManager->post(request, postData);
    connect(reply, &QNetworkReply::finished, this, &HttpSender::onPublicKeyReplyFinished);
}

void HttpSender::onPublicKeyReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "获取服务端公钥失败:" << reply->errorString();
        reply->deleteLater();
        return;
    }
    
    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isObject()) {
        qWarning() << "服务端公钥响应格式错误";
        return;
    }
    
    QJsonObject obj = doc.object();
    QString publicKeyStr = obj.value("data").toString();
    
    if (publicKeyStr.isEmpty()) {
        qWarning() << "获取服务端公钥失败: 返回为空";
        return;
    }
    
    if (publicKeyStr.startsWith("\"")) {
        publicKeyStr = publicKeyStr.mid(1);
    }
    if (publicKeyStr.endsWith("\"")) {
        publicKeyStr = publicKeyStr.left(publicKeyStr.length() - 1);
    }
    
    qInfo() << "获得服务端公钥:" << publicKeyStr;
    m_servicePublicKey = QByteArray::fromBase64(publicKeyStr.toUtf8());
    
    m_initialized = true;
    emit publicKeyReceived();
    
    QJsonObject readSetting;
    readSetting["cmd"] = "readSetting";
    emit responseReceived(QJsonDocument(readSetting).toJson());
    
    QJsonObject readVersion;
    readVersion["cmd"] = "readVersion";
    emit responseReceived(QJsonDocument(readVersion).toJson());
}

void HttpSender::onDataReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "发送数据失败:" << reply->errorString();
        emit sendFailed(reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    
    if (!responseData.isEmpty()) {
        emit responseReceived(responseData);
    }
    
    emit sendSuccess();
}

void HttpSender::getBoxRsaKeys() {
    DatabaseManager* db = DatabaseManager::instance();
    
    QString publicKeyStr = db->getKeyValue(BOX_PUBLIC_KEY).toString();
    QString privateKeyStr = db->getKeyValue(BOX_PRIVATE_KEY).toString();
    
    if (publicKeyStr.isEmpty() || privateKeyStr.isEmpty()) {
        qInfo() << "生成新的 RSA 密钥对";
        RSAUtils::KeyPair keys = RSAUtils::generateKeyPair();
        m_boxPublicKey = keys.publicKey;
        m_boxPrivateKey = keys.privateKey;
        
        db->saveKeyValue(BOX_PRIVATE_KEY, m_boxPrivateKey.toBase64());
        db->saveKeyValue(BOX_PUBLIC_KEY, m_boxPublicKey.toBase64());
    } else {
        m_boxPrivateKey = QByteArray::fromBase64(privateKeyStr.toUtf8());
        m_boxPublicKey = QByteArray::fromBase64(publicKeyStr.toUtf8());
    }
    
    qInfo() << "水箱公钥:" << publicKeyStr << ", 私钥:" << privateKeyStr;
}

QString HttpSender::getUniqueId() {
    DatabaseManager* db = DatabaseManager::instance();
    QString id = db->getKeyValue(BOX_ADDRESS).toString();
    
    if (id.isEmpty()) {
        QString macAddress = NetworkUtils::getFirstMacAddress();
        QString staticString = getStaticString();
        
        qInfo() << "MAC地址:" << macAddress << ", 静态字符串:" << staticString;
        
        id = NetworkUtils::generateUniqueId(macAddress, staticString);
        db->saveKeyValue(BOX_ADDRESS, id);
    }
    
    qInfo() << "水箱ID:" << id;
    return id;
}

QString HttpSender::getStaticString() {
    DatabaseManager* db = DatabaseManager::instance();
    QString ret = db->getKeyValue(STATIC_STRING).toString();
    
    if (ret.isEmpty()) {
        ret = NetworkUtils::generateUUID();
        db->saveKeyValue(STATIC_STRING, ret);
    }
    
    return ret;
}
