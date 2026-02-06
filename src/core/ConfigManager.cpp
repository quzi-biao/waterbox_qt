#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

ConfigManager* ConfigManager::m_instance = nullptr;

ConfigManager::ConfigManager() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configPath);
    m_configFile = configPath + "/config.json";
    
    // 基本配置
    m_config["service_address"] = "http://up.waters-ai.work";
    m_config["tag"] = "";
    
    // PLC 配置
    m_config["plc_host"] = "192.168.1.11";
    m_config["plc_port"] = 102;
    m_config["plc_protocol"] = "S7";
    m_config["plcServer"] = true;
    m_config["plcLocalHost"] = "192.168.1.11";
    m_config["plcSimulate"] = false;
    
    // 采样配置
    m_config["collect_interval"] = 10000;
    m_config["report_interval"] = 10000;
    m_config["sampleInterval"] = 10;
    
    // 泵组监控配置
    m_config["openMetric"] = true;
    m_config["pumpMetricReadInterval"] = 10000;
    m_config["regionNumber"] = 1;
    m_config["pumpNumber"] = 1;
    
    // 其他默认配置
    m_config["sendType"] = "http";
    m_config["rtuProtocol"] = "rtu";
    m_config["productType"] = 1;
}

ConfigManager::~ConfigManager() {
}

ConfigManager* ConfigManager::instance() {
    if (!m_instance) {
        m_instance = new ConfigManager();
    }
    return m_instance;
}

QVariant ConfigManager::get(const QString& key, const QVariant& defaultValue) const {
    QMutexLocker locker(&m_mutex);
    return m_config.value(key, defaultValue);
}

void ConfigManager::set(const QString& key, const QVariant& value) {
    QMutexLocker locker(&m_mutex);
    m_config[key] = value;
    emit configChanged(key);
}

bool ConfigManager::load() {
    QFile file(m_configFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file:" << m_configFile;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }
    
    QJsonObject obj = doc.object();
    QMutexLocker locker(&m_mutex);
    
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        m_config[it.key()] = it.value().toVariant();
    }
    
    return true;
}

bool ConfigManager::save() {
    QMutexLocker locker(&m_mutex);
    
    QJsonObject obj;
    for (auto it = m_config.begin(); it != m_config.end(); ++it) {
        obj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    
    QJsonDocument doc(obj);
    
    QFile file(m_configFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save config file:" << m_configFile;
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    return true;
}

void ConfigManager::updateFromRemote(const QJsonObject& remoteConfig) {
    for (auto it = remoteConfig.begin(); it != remoteConfig.end(); ++it) {
        QString key = it.key();
        QVariant value = it.value().toVariant();
        
        set(key, value);
        
        if (key.startsWith("plc_write_")) {
            QString address = key.mid(10);
            emit plcAddressWriteRequired(address, value);
        }
    }
    
    save();
}

QString ConfigManager::plcHost() const {
    return get("plc_host").toString();
}

int ConfigManager::plcPort() const {
    return get("plc_port").toInt();
}

QString ConfigManager::plcProtocol() const {
    return get("plc_protocol").toString();
}

QString ConfigManager::reportUrl() const {
    return get("report_url").toString();
}

QString ConfigManager::reportProtocol() const {
    return get("report_protocol").toString();
}

int ConfigManager::collectInterval() const {
    return get("collect_interval", 10000).toInt();
}

int ConfigManager::reportInterval() const {
    return get("report_interval", 10000).toInt();
}

QJsonObject ConfigManager::dataSchema() const {
    QVariant schema = get("data_schema");
    if (schema.canConvert<QJsonObject>()) {
        return schema.toJsonObject();
    }
    return QJsonObject();
}

QMap<QString, bool> ConfigManager::addressWritePermissions() const {
    QMap<QString, bool> permissions;
    QMutexLocker locker(&m_mutex);
    
    for (auto it = m_config.begin(); it != m_config.end(); ++it) {
        if (it.key().startsWith("addr_writable_")) {
            QString address = it.key().mid(14);
            permissions[address] = it.value().toBool();
        }
    }
    
    return permissions;
}
