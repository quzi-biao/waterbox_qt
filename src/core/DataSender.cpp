#include "DataSender.h"
#include "network/HttpSender.h"
#include "network/HttpSendData.h"
#include "database/DatabaseManager.h"
#include "entity/MetricIndicator.h"
#include "core/DataCollector.h"
#include "ConfigManager.h"
#include <QDebug>
#include <QThread>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHash>
#include <QMap>

DataSender::DataSender(HttpSender* sender, QObject* parent)
    : QObject(parent), 
      m_sender(sender),
      m_collector(nullptr),
      m_baseInterval(10000),
      m_minInterval(1000),
      m_currentInterval(10000),
      m_started(false) {
    
    m_sendTimer = new QTimer(this);
    connect(m_sendTimer, &QTimer::timeout, this, &DataSender::sendPendingData);
    
    m_adjustTimer = new QTimer(this);
    m_adjustTimer->setInterval(5000);
    connect(m_adjustTimer, &QTimer::timeout, this, &DataSender::adjustSendFrequency);

    if (m_sender) {
        connect(m_sender, &HttpSender::publicKeyReceived, this, &DataSender::sendPendingData);
    }
    
    m_baseInterval = ConfigManager::instance()->reportInterval();
    m_currentInterval = m_baseInterval;
    m_sendTimer->setInterval(m_currentInterval);
}

void DataSender::start() {
    if (m_started) {
        return;
    }
    
    qInfo() << "Starting data sender";
    m_started = true;
    m_sendTimer->start();
    m_adjustTimer->start();
    
    sendPendingData();
}

void DataSender::stop() {
    qInfo() << "Stopping data sender";
    m_started = false;
    m_sendTimer->stop();
    m_adjustTimer->stop();
}

void DataSender::setInterval(int milliseconds) {
    m_baseInterval = milliseconds;
    m_currentInterval = milliseconds;
    m_sendTimer->setInterval(milliseconds);
}

void DataSender::addData(const QJsonObject& data) {
    if (!m_started) {
        return;
    }
}

void DataSender::sendPendingData() {
    qDebug() << "DataSender::sendPendingData() called";
    
    if (!m_sender || !m_sender->isInitialized()) {
        return;
    }
    
    QList<PLCDataRecord> records = DatabaseManager::instance()->getUnuploadedData(100);
    
    qInfo() << "Found" << records.size() << "unuploaded records in database";
    
    if (records.isEmpty()) {
        qDebug() << "No pending data to send";
        return;
    }
    
    // 从DataCollector获取MetricIndicator配置，建立address到id的映射
    QList<MetricIndicator> indicators;
    if (m_collector) {
        indicators = m_collector->getMetricIndicators();
    } else {
        indicators = DatabaseManager::instance()->loadMetricIndicators();
    }
    
    QMap<QString, qint64> addressToIdMap;
    for (const MetricIndicator& indicator : indicators) {
        addressToIdMap[indicator.address()] = indicator.id();
        qDebug() << "  Mapping:" << indicator.address() << "->" << indicator.id();
    }
    
    qInfo() << "Loaded" << indicators.size() << "metric indicators from" << (m_collector ? "DataCollector" : "Database");
    qDebug() << "Address map size:" << addressToIdMap.size();
    
    // 转换为QList<QJsonObject>格式，匹配Java的readIndicator返回格式
    // Java: ret.put("k", indicator.getId()); ret.put("v", value); ret.put("t", timestamp);
    QList<QJsonObject> dataList;
    QList<qint64> recordIds;
    
    for (const PLCDataRecord& record : records) {
        if (!addressToIdMap.contains(record.address)) {
            qWarning() << "Address not found in MetricIndicator:" << record.address;
            continue;
        }

        QJsonObject metric;
        metric["k"] = static_cast<qint64>(addressToIdMap[record.address]);  // k = MetricIndicator的id
        metric["v"] = record.correctedValue.toDouble();  // v = 值
        metric["t"] = record.timestamp.toMSecsSinceEpoch();  // t = 时间戳（毫秒）

        dataList.append(metric);
        recordIds.append(record.id);
    }
    
    if (dataList.isEmpty()) {
        qWarning() << "No metrics to send after address-id mapping";
        return;
    }

    qInfo() << "Preparing to send" << dataList.size() << "metrics in one batch";
    
    // 使用HttpSendData::createMetricData创建数据（完全匹配Java实现）
    QString uniqueId = m_sender->uniqueId();
    if (uniqueId.isEmpty()) {
        qWarning() << "UniqueId is empty! Cannot send data.";
        return;
    }
    
    QString tag = ConfigManager::instance()->get("tag", "").toString();
    HttpSendData httpData = HttpSendData::createMetricData(
        uniqueId, dataList, QByteArray(), QByteArray(), tag);
    
    if (httpData.content().isEmpty()) {
        qWarning() << "Failed to create metric data";
        return;
    }
    
    qInfo() << "Sending batch with" << dataList.size() << "metrics";
    
    // 输出前5个指标用于调试
    qDebug() << "Sample metrics (first 5):";
    for (int i = 0; i < qMin(5, dataList.size()); i++) {
        QJsonObject metric = dataList[i];
        qDebug() << "  k=" << metric["k"].toVariant().toLongLong()
                 << " v=" << metric["v"].toDouble()
                 << " t=" << metric["t"].toVariant().toLongLong();
    }
    
    qDebug() << "Content:" << httpData.content().left(100);
    
    // 转换为JSON并发送
    QJsonObject sendData = httpData.toJson();
    QJsonDocument debugDoc(sendData);
    qInfo() << "Sending data:" << debugDoc.toJson(QJsonDocument::Compact).left(300);
    
    // 使用 HttpSender 发送
    m_sender->sendRawData(sendData);
    
    // 标记所有数据为已上传
    for (qint64 id : recordIds) {
        DatabaseManager::instance()->markAsUploaded(id);
        emit dataSent(id);
    }
    qInfo() << "Successfully sent" << dataList.size() << "metrics";
}

void DataSender::adjustSendFrequency() {
    QList<PLCDataRecord> pending = DatabaseManager::instance()->getUnuploadedData(10);
    
    if (pending.size() > 1) {
        int newInterval = qMax(m_minInterval, m_currentInterval / 2);
        if (newInterval != m_currentInterval) {
            m_currentInterval = newInterval;
            m_sendTimer->setInterval(m_currentInterval);
            qInfo() << "Increased send frequency to" << m_currentInterval << "ms";
        }
    } else if (pending.isEmpty()) {
        if (m_currentInterval < m_baseInterval) {
            m_currentInterval = qMin(m_baseInterval, m_currentInterval * 2);
            m_sendTimer->setInterval(m_currentInterval);
            qInfo() << "Decreased send frequency to" << m_currentInterval << "ms";
        }
    }
}
