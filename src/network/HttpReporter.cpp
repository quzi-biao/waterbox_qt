#include "HttpReporter.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

HttpReporter::HttpReporter(QObject* parent)
    : QObject(parent), m_connected(false) {
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::finished, 
            this, &HttpReporter::onReplyFinished);
}

HttpReporter::~HttpReporter() {
    closeConnection();
}

bool HttpReporter::establishConnection(const QString& url) {
    m_url = url;
    m_connected = true;
    qInfo() << "HTTP Reporter configured for URL:" << url;
    return true;
}

bool HttpReporter::reportData(const QJsonObject& data) {
    if (!m_connected || m_url.isEmpty()) {
        qWarning() << "HTTP Reporter not connected";
        return false;
    }
    
    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonDocument doc(data);
    QByteArray jsonData = doc.toJson();
    
    qDebug() << "Sending HTTP POST to:" << m_url;
    qDebug() << "Data:" << jsonData.left(200);  // 只显示前200字节
    
    QNetworkReply* reply = m_manager->post(request, jsonData);
    
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timer.start(10000);
    loop.exec();
    
    if (timer.isActive()) {
        timer.stop();
        
        if (reply->error() == QNetworkReply::NoError) {
            m_lastResponse = reply->readAll();
            qInfo() << "HTTP report SUCCESS - Response:" << m_lastResponse.left(100);
            reply->deleteLater();
            emit reportSuccess();
            return true;
        } else {
            qWarning() << "HTTP report FAILED:" << reply->errorString();
            qWarning() << "Error code:" << reply->error();
            qWarning() << "URL:" << m_url;
            emit reportFailed(reply->errorString());
        }
    } else {
        qWarning() << "HTTP report TIMEOUT after 10 seconds";
        qWarning() << "URL:" << m_url;
        reply->abort();
        emit reportFailed("Timeout");
    }
    
    reply->deleteLater();
    return false;
}

void HttpReporter::closeConnection() {
    m_connected = false;
    m_url.clear();
}

bool HttpReporter::isConnected() const {
    return m_connected;
}

QString HttpReporter::getLastResponse() const {
    return m_lastResponse;
}

void HttpReporter::onReplyFinished(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        m_lastResponse = reply->readAll();
        emit responseReceived(m_lastResponse);
    }
}
