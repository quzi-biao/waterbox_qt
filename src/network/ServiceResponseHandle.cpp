#include "ServiceResponseHandle.h"
#include "HttpSender.h"
#include "CommandHandler.h"
#include "RemoteConfigReceiver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QDebug>

QQueue<QString> ServiceResponseHandle::s_responses;

ServiceResponseHandle::ServiceResponseHandle(QObject* parent)
    : QObject(parent),
      m_timer(new QTimer(this)),
      m_sender(nullptr),
      m_commandHandler(new CommandHandler(this)) {
    
    connect(m_timer, &QTimer::timeout, this, &ServiceResponseHandle::processResponses);
}

ServiceResponseHandle::~ServiceResponseHandle() {
}

void ServiceResponseHandle::initialize(HttpSender* sender) {
    m_sender = sender;
    m_commandHandler->initialize(sender);
    m_timer->start(5000);
}

void ServiceResponseHandle::addResponse(const QByteArray& data) {
    if (!data.isEmpty()) {
        s_responses.enqueue(QString::fromUtf8(data));
    }
}

void ServiceResponseHandle::processResponses() {
    while (!s_responses.isEmpty()) {
        QString data = s_responses.dequeue();
        try {
            processResponse(data);
            QThread::msleep(200);
        } catch (...) {
            qWarning() << "处理响应时发生异常";
            break;
        }
    }
}

void ServiceResponseHandle::processResponse(const QString& data) {
    if (data.isEmpty()) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isObject()) {
        qWarning() << "响应数据格式错误:" << data;
        return;
    }
    
    QJsonObject obj = doc.object();
    QString cmd = obj.value("cmd").toString();
    
    if (cmd.isEmpty()) {
        return;
    }
    
    qInfo() << "处理指令:" << cmd;
    
    m_commandHandler->handleCommand(obj);
}
