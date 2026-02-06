#include "RemoteConfigReceiver.h"
#include "core/ConfigManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

RemoteConfigReceiver::RemoteConfigReceiver(QObject* parent)
    : QObject(parent) {
}

void RemoteConfigReceiver::processResponse(const QString& response) {
    if (response.isEmpty()) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    if (!doc.isObject()) {
        return;
    }
    
    QJsonObject obj = doc.object();
    
    if (obj.contains("cmd")) {
        handleCommand(obj);
    }
    
    if (obj.contains("commands") && obj["commands"].isArray()) {
        QJsonArray commands = obj["commands"].toArray();
        for (const QJsonValue& val : commands) {
            if (val.isObject()) {
                handleCommand(val.toObject());
            }
        }
    }
}

void RemoteConfigReceiver::handleCommand(const QJsonObject& cmd) {
    QString cmdType = cmd["cmd"].toString();
    
    qInfo() << "Processing command:" << cmdType;
    
    if (cmdType == "update_config") {
        QJsonObject config = cmd["data"].toObject();
        emit configReceived(config);
        ConfigManager::instance()->updateFromRemote(config);
        
    } else if (cmdType == "write_plc") {
        QString address = cmd["address"].toString();
        QVariant value = cmd["value"].toVariant();
        emit plcWriteCommand(address, value);
        
    } else if (cmdType == "set_interval") {
        int interval = cmd["interval"].toInt();
        ConfigManager::instance()->set("collect_interval", interval);
        
    } else if (cmdType == "set_report_url") {
        QString url = cmd["url"].toString();
        ConfigManager::instance()->set("report_url", url);
        
    } else {
        qWarning() << "Unknown command:" << cmdType;
    }
}
