#include "DataCollector.h"
#include "StressTestDataGenerator.h"
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
#include <QRandomGenerator>

DataCollector::DataCollector(IPLCClient* plcClient, QObject* parent)
    : QObject(parent), 
      m_plcClient(plcClient),
      m_stressTestMode(false),
      m_stressGenerator(new StressTestDataGenerator()) {
    
    // 初始化时加载配置
    DatabaseManager* db = DatabaseManager::instance();
    QVariant indicatorValue = db->getKeyValue("METRIC_INDICATOR_KEY");
    
    if (!indicatorValue.isNull() && !indicatorValue.toString().isEmpty()) {
        QString jsonStr = indicatorValue.toString();
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            for (const QJsonValue& val : array) {
                if (val.isObject()) {
                    QJsonObject obj = val.toObject();
                    QString address = obj.value("address").toString();
                    if (!address.isEmpty()) {
                        m_dataSchema[address] = QString::number(obj.value("dataType").toInt());
                    }
                }
            }
        }
    }
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DataCollector::collectData);
    
    // 设置 10 秒采集间隔
    m_timer->setInterval(10000);
}

DataCollector::~DataCollector() {
    stopCollection();
}

void DataCollector::startCollection() {
    m_timer->start();
    // 立即采集一次
    collectData();
}

void DataCollector::stopCollection() {
    m_timer->stop();
}

void DataCollector::setInterval(int milliseconds) {
    m_timer->setInterval(milliseconds);
    qInfo() << "Collection interval set to" << milliseconds << "ms";
}

void DataCollector::setDataSchema(const QMap<QString, QString>& schema) {
    m_dataSchema = schema;
}

QMap<QString, QVariant> DataCollector::getLatestData() const {
    return m_latestData;
}

QList<MetricIndicator> DataCollector::getMetricIndicators() const {
    return DatabaseManager::instance()->loadMetricIndicators();
}

void DataCollector::setStressTestMode(bool enabled) {
    m_stressTestMode = enabled;
    if (enabled) {
        m_stressGenerator->initialize();
        m_dataSchema = m_stressGenerator->dataSchema();
        qInfo() << "压测模式已开启";
    } else {
        m_stressGenerator->reset();
        qInfo() << "压测模式已关闭";
    }
}

bool DataCollector::isStressTestMode() const {
    return m_stressTestMode;
}

void DataCollector::collectStressTestData() {
    QMap<QString, QVariant> data = m_stressGenerator->generateData();
    m_latestData = data;
    emit dataCollected(data);
}

void DataCollector::collectData() {
    // 压测模式：不访问 PLC，生成模拟数据
    if (m_stressTestMode) {
        collectStressTestData();
        return;
    }
    
    // 重新加载配置（以防配置更新）
    if (m_dataSchema.isEmpty()) {
        // 配置为空，跳过采集
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
        } else {
            successCount++;
            collectedData[address] = rawValue;
            
            // 保存到数据库
            DatabaseManager::instance()->saveData(address, rawValue, rawValue);
            
        }
    }
    
    // 添加时间戳
    collectedData["_timestamp"] = timestamp;
    
    m_latestData = collectedData;
    
    Q_UNUSED(successCount);
    Q_UNUSED(failCount);
    
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
