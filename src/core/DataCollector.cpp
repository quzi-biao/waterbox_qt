#include "DataCollector.h"
#include "interfaces/IPLCClient.h"
#include "plc/PLCClient.h"
#include "database/DatabaseManager.h"
#include "ConfigManager.h"
#include "entity/MetricIndicator.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

DataCollector::DataCollector(IPLCClient* plcClient, QObject* parent)
    : QObject(parent), 
      m_plcClient(plcClient) {
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DataCollector::collectData);
    
    // 设置 10 秒采集间隔
    m_timer->setInterval(10000);
    
    // 加载 MetricIndicator 配置
    loadMetricIndicators();
}

DataCollector::~DataCollector() {
    stopCollection();
}

void DataCollector::startCollection() {
    qInfo() << "Starting data collection";
    m_timer->start();
    // 立即采集一次
    collectData();
}

void DataCollector::stopCollection() {
    qInfo() << "Stopping data collection";
    m_timer->stop();
}

void DataCollector::setInterval(int milliseconds) {
    m_timer->setInterval(milliseconds);
    qInfo() << "Collection interval set to" << milliseconds << "ms";
}

void DataCollector::setDataSchema(const QMap<QString, QString>& schema) {
    m_dataSchema = schema;
}

void DataCollector::loadMetricIndicators() {
    m_dataSchema.clear();
    
    DatabaseManager* db = DatabaseManager::instance();
    QVariant indicatorValue = db->getKeyValue("METRIC_INDICATOR_KEY");
    
    if (indicatorValue.isNull() || indicatorValue.toString().isEmpty()) {
        qWarning() << "没有配置 PLC 监控地址";
        return;
    }
    
    QString jsonStr = indicatorValue.toString();
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isArray()) {
        qWarning() << "MetricIndicator 配置格式错误";
        return;
    }
    
    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        if (!val.isObject()) {
            continue;
        }
        
        MetricIndicator indicator = MetricIndicator::fromJson(val.toObject());
        QString address = indicator.address();
        
        if (!address.isEmpty()) {
            // 使用地址作为 key，数据类型作为 value
            m_dataSchema[address] = QString::number(indicator.dataType());
        }
    }
    
    qInfo() << "加载了" << m_dataSchema.size() << "个 PLC 监控地址";
}

QMap<QString, QVariant> DataCollector::getLatestData() const {
    return m_latestData;
}

void DataCollector::collectData() {
    // 重新加载配置（以防配置更新）
    if (m_dataSchema.isEmpty()) {
        loadMetricIndicators();
    }
    
    if (m_dataSchema.isEmpty()) {
        qWarning() << "没有配置 PLC 监控地址，跳过数据采集";
        emit collectionError("No PLC addresses configured");
        return;
    }
    
    if (!m_plcClient) {
        emit collectionError("PLC client not initialized");
        return;
    }
    
    // 从数据库重新加载完整的 MetricIndicator 列表
    DatabaseManager* db = DatabaseManager::instance();
    QVariant indicatorValue = db->getKeyValue("METRIC_INDICATOR_KEY");
    
    if (indicatorValue.isNull() || indicatorValue.toString().isEmpty()) {
        qWarning() << "无法读取 MetricIndicator 配置";
        emit collectionError("No MetricIndicator configuration");
        return;
    }
    
    QString jsonStr = indicatorValue.toString();
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isArray()) {
        qWarning() << "MetricIndicator 配置格式错误";
        return;
    }
    
    QJsonArray indicators = doc.array();
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    
    qInfo() << "开始采集" << indicators.size() << "个 PLC 地址的数据";
    
    QMap<QString, QVariant> collectedData;  // 用于界面显示
    int successCount = 0;
    int failCount = 0;
    
    for (const QJsonValue& val : indicators) {
        if (!val.isObject()) {
            continue;
        }
        
        QJsonObject indicatorObj = val.toObject();
        qint64 id = indicatorObj.value("id").toVariant().toLongLong();
        QString address = indicatorObj.value("address").toString();
        int dataType = indicatorObj.value("dataType").toInt();
        
        if (address.isEmpty()) {
            continue;
        }
        
        // 读取数据
        QVariant rawValue = readIndicatorValue(address, dataType);
        
        if (!rawValue.isValid() || rawValue.isNull()) {
            failCount++;
            qDebug() << "读取失败:" << address << "类型:" << dataType;
        } else {
            successCount++;
            collectedData[address] = rawValue;
            
            // 保存到数据库
            DatabaseManager::instance()->saveData(address, rawValue, rawValue);
            
            qDebug() << "读取成功:" << address << "=" << rawValue << "类型:" << dataType;
        }
    }
    
    // 添加时间戳
    collectedData["_timestamp"] = timestamp;
    
    m_latestData = collectedData;
    
    qInfo() << "数据采集完成，共" << collectedData.size() << "项，成功:" << successCount << "失败:" << failCount;
    
    // 如果失败太多，发送错误信号
    if (successCount == 0 && failCount > 0) {
        emit collectionError(QString("All %1 addresses failed to read").arg(failCount));
    } else {
        emit dataCollected(collectedData);
    }
}

QVariant DataCollector::readIndicatorValue(const QString& address, int dataType) {
    // 尝试转换为 PLCClient 以使用带类型的读取方法
    PLCClient* plcClient = dynamic_cast<PLCClient*>(m_plcClient);
    if (plcClient) {
        QVariant value = plcClient->readDataWithType(address, dataType);
        if (!value.isValid() || value.isNull()) {
            return QVariant();
        }
        return value;
    }
    
    // 回退到普通读取
    return m_plcClient->readData(address);
}

QVariant DataCollector::correctValue(const QString& address, const QVariant& rawValue) {
    double value = rawValue.toDouble();
    
    if (m_lastValidData.contains(address)) {
        double lastValue = m_lastValidData[address].toDouble();
        double diff = qAbs(value - lastValue);
        
        if (diff > lastValue * 0.5 && lastValue > 0) {
            qWarning() << "Abnormal value detected for" << address 
                      << "- raw:" << value << "last:" << lastValue;
            return lastValue;
        }
    }
    
    return rawValue;
}

QVariant DataCollector::fillMissingValue(const QString& address) {
    if (m_lastValidData.contains(address)) {
        return m_lastValidData[address];
    }
    
    return QVariant();
}
