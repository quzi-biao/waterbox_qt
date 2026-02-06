#include "DatabaseManager.h"
#include "entity/MetricIndicator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

DatabaseManager* DatabaseManager::m_instance = nullptr;

DatabaseManager::DatabaseManager() {
}

DatabaseManager::~DatabaseManager() {
    close();
}

DatabaseManager* DatabaseManager::instance() {
    if (!m_instance) {
        m_instance = new DatabaseManager();
    }
    return m_instance;
}

bool DatabaseManager::initialize() {
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbPath);
    
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath + "/waterbox.db");
    
    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }
    
    return createTables();
}

void DatabaseManager::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::createTables() {
    QSqlQuery query(m_db);
    
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS plc_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp INTEGER NOT NULL, "
        "address TEXT NOT NULL, "
        "raw_value TEXT, "
        "corrected_value TEXT, "
        "uploaded INTEGER DEFAULT 0"
        ")"
    );
    
    if (!success) {
        qCritical() << "Failed to create plc_data table:" << query.lastError().text();
        return false;
    }
    
    query.exec("CREATE INDEX IF NOT EXISTS idx_timestamp ON plc_data(timestamp)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_uploaded ON plc_data(uploaded)");
    
    success = query.exec(
        "CREATE TABLE IF NOT EXISTS kv_storage ("
        "key TEXT PRIMARY KEY, "
        "value TEXT"
        ")"
    );
    
    if (!success) {
        qCritical() << "Failed to create kv_storage table:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::saveData(const QString& address, const QVariant& rawValue, const QVariant& correctedValue) {
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO plc_data (timestamp, address, raw_value, corrected_value, uploaded) "
        "VALUES (:timestamp, :address, :raw_value, :corrected_value, 0)"
    );
    
    query.bindValue(":timestamp", QDateTime::currentMSecsSinceEpoch());
    query.bindValue(":address", address);
    query.bindValue(":raw_value", rawValue.toString());
    query.bindValue(":corrected_value", correctedValue.toString());
    
    if (!query.exec()) {
        qWarning() << "Failed to save data:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::markAsUploaded(qint64 id) {
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_db);
    query.prepare("UPDATE plc_data SET uploaded = 1 WHERE id = :id");
    query.bindValue(":id", id);
    
    return query.exec();
}

QList<PLCDataRecord> DatabaseManager::getUnuploadedData(int limit) {
    QMutexLocker locker(&m_mutex);
    
    QList<PLCDataRecord> records;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT id, timestamp, address, raw_value, corrected_value, uploaded "
                  "FROM plc_data WHERE uploaded = 0 ORDER BY id LIMIT :limit");
    query.bindValue(":limit", limit);
    
    if (!query.exec()) {
        qWarning() << "Failed to query unuploaded data:" << query.lastError().text();
        return records;
    }
    
    while (query.next()) {
        PLCDataRecord record;
        record.id = query.value(0).toLongLong();
        record.timestamp = QDateTime::fromMSecsSinceEpoch(query.value(1).toLongLong());
        record.address = query.value(2).toString();
        record.rawValue = query.value(3);
        record.correctedValue = query.value(4);
        record.uploaded = query.value(5).toBool();
        records.append(record);
    }
    
    return records;
}

QList<PLCDataRecord> DatabaseManager::getHistoryData(const QString& address, const QDateTime& start, const QDateTime& end) {
    QMutexLocker locker(&m_mutex);
    
    QList<PLCDataRecord> records;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT id, timestamp, address, raw_value, corrected_value, uploaded "
                  "FROM plc_data WHERE address = :address AND timestamp >= :start AND timestamp <= :end "
                  "ORDER BY timestamp");
    query.bindValue(":address", address);
    query.bindValue(":start", start.toMSecsSinceEpoch());
    query.bindValue(":end", end.toMSecsSinceEpoch());
    
    if (!query.exec()) {
        qWarning() << "Failed to query history data:" << query.lastError().text();
        return records;
    }
    
    while (query.next()) {
        PLCDataRecord record;
        record.id = query.value(0).toLongLong();
        record.timestamp = QDateTime::fromMSecsSinceEpoch(query.value(1).toLongLong());
        record.address = query.value(2).toString();
        record.rawValue = query.value(3);
        record.correctedValue = query.value(4);
        record.uploaded = query.value(5).toBool();
        records.append(record);
    }
    
    return records;
}

bool DatabaseManager::saveKeyValue(const QString& key, const QVariant& value) {
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO kv_storage (key, value) VALUES (:key, :value)");
    query.bindValue(":key", key);
    query.bindValue(":value", value.toString());
    
    return query.exec();
}

QVariant DatabaseManager::getKeyValue(const QString& key, const QVariant& defaultValue) {
    QMutexLocker locker(&m_mutex);
    
    QSqlQuery query(m_db);
    query.prepare("SELECT value FROM kv_storage WHERE key = :key");
    query.bindValue(":key", key);
    
    if (query.exec() && query.next()) {
        return query.value(0);
    }
    
    return defaultValue;
}

QMap<QString, QVariant> DatabaseManager::getLatestData() {
    QMutexLocker locker(&m_mutex);
    
    QMap<QString, QVariant> latestData;
    
    QSqlQuery query(m_db);
    // 获取每个地址的最新记录
    query.prepare(
        "SELECT address, corrected_value, MAX(timestamp) as max_time "
        "FROM plc_data "
        "GROUP BY address "
        "ORDER BY max_time DESC"
    );
    
    if (!query.exec()) {
        qWarning() << "Failed to query latest data:" << query.lastError().text();
        return latestData;
    }
    
    while (query.next()) {
        QString address = query.value(0).toString();
        QVariant value = query.value(1);
        latestData[address] = value;
    }
    
    // 添加时间戳
    if (!latestData.isEmpty()) {
        QSqlQuery timeQuery(m_db);
        timeQuery.prepare("SELECT MAX(timestamp) FROM plc_data");
        if (timeQuery.exec() && timeQuery.next()) {
            latestData["_timestamp"] = timeQuery.value(0);
        }
    }
    
    return latestData;
}

QList<MetricIndicator> DatabaseManager::loadMetricIndicators() {
    QList<MetricIndicator> indicators;

    // 使用与 PLCAddressConfigWidget 相同的读取方式（getKeyValue 内部加锁）
    QVariant indicatorValue = getKeyValue("METRIC_INDICATOR_KEY");
    if (indicatorValue.isNull() || indicatorValue.toString().isEmpty()) {
        return indicators;
    }

    QString jsonStr = indicatorValue.toString();
    
    // 解析JSON数组
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isArray()) {
        qWarning() << "Invalid metric indicators JSON format";
        return indicators;
    }
    
    QJsonArray array = doc.array();
    for (const QJsonValue& value : array) {
        if (value.isObject()) {
            indicators.append(MetricIndicator::fromJson(value.toObject()));
        }
    }
    
    return indicators;
}

void DatabaseManager::cleanOldData(int daysToKeep) {
    QMutexLocker locker(&m_mutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-daysToKeep);
    
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM plc_data WHERE timestamp < :cutoff");
    query.bindValue(":cutoff", cutoff.toMSecsSinceEpoch());
    
    if (query.exec()) {
        qInfo() << "Cleaned old data before" << cutoff;
    }
}
