#include "DataSender.h"
#include "interfaces/IDataReporter.h"
#include "database/DatabaseManager.h"
#include "ConfigManager.h"
#include <QDebug>
#include <QThread>

DataSender::DataSender(IDataReporter* reporter, QObject* parent)
    : QObject(parent), 
      m_reporter(reporter),
      m_baseInterval(10000),
      m_minInterval(1000),
      m_currentInterval(10000),
      m_started(false) {
    
    m_sendTimer = new QTimer(this);
    connect(m_sendTimer, &QTimer::timeout, this, &DataSender::sendPendingData);
    
    m_adjustTimer = new QTimer(this);
    m_adjustTimer->setInterval(5000);
    connect(m_adjustTimer, &QTimer::timeout, this, &DataSender::adjustSendFrequency);
    
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
    if (!m_reporter || !m_reporter->isConnected()) {
        qWarning() << "Reporter not connected";
        return;
    }
    
    QList<PLCDataRecord> records = DatabaseManager::instance()->getUnuploadedData(100);
    
    if (records.isEmpty()) {
        return;
    }
    
    qInfo() << "Sending" << records.size() << "pending records";
    
    for (const PLCDataRecord& record : records) {
        QJsonObject data;
        data["id"] = record.id;
        data["timestamp"] = record.timestamp.toMSecsSinceEpoch();
        data["address"] = record.address;
        data["raw_value"] = record.rawValue.toString();
        data["corrected_value"] = record.correctedValue.toString();
        
        if (m_reporter->reportData(data)) {
            DatabaseManager::instance()->markAsUploaded(record.id);
            emit dataSent(record.id);
            
            QThread::msleep(200);
        } else {
            emit sendError("Failed to send data");
            break;
        }
    }
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
