#include "PressureController.h"
#include "interfaces/IPLCClient.h"
#include "database/DatabaseManager.h"
#include "ConfigManager.h"
#include <QDateTime>
#include <QDebug>
#include <QtMath>

PressureController::PressureController(IPLCClient* plcClient, QObject* parent)
    : QObject(parent),
      m_plcClient(plcClient),
      m_enabled(false),
      m_defaultPressure(0.3),
      m_minPressure(0.2),
      m_maxPressure(0.5),
      m_pressureIncreaseInterval(60),
      m_lastWriteTime(0),
      m_lastWritePressure(0),
      m_lastTargetPressure(0) {
    
    m_controlPressureAddress = ConfigManager::instance()->get("ctrl_pressure_addr", "V100").toString();
    m_endPressureAddress = ConfigManager::instance()->get("end_pressure_addr", "V200").toString();
    m_flowAddress = ConfigManager::instance()->get("flow_addr", "V300").toString();
}

QMap<QString, QVariant> PressureController::readData() {
    QMap<QString, QVariant> data;
    
    if (!m_plcClient || !m_plcClient->isConnected()) {
        return data;
    }
    
    data["end_pressure"] = m_plcClient->readData(m_endPressureAddress);
    data["flow"] = m_plcClient->readData(m_flowAddress);
    data["current_ctrl_pressure"] = m_plcClient->readData(m_controlPressureAddress);
    
    return data;
}

QMap<QString, QVariant> PressureController::processData(const QMap<QString, QVariant>& inputData) {
    QMap<QString, QVariant> controlData;
    
    if (!m_enabled) {
        return controlData;
    }
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_lastWriteTime > 0 && 
        (currentTime - m_lastWriteTime) < m_pressureIncreaseInterval * 1000) {
        qDebug() << "Skipping pressure control - interval not reached";
        return controlData;
    }
    
    double targetPressure = calculateTargetPressure(inputData);
    double writePressure = calculateWritePressure(targetPressure);
    
    controlData["target_pressure"] = targetPressure;
    controlData["write_pressure"] = writePressure;
    controlData["control_address"] = m_controlPressureAddress;
    
    m_lastTargetPressure = targetPressure;
    m_lastWritePressure = writePressure;
    
    emit pressureCalculated(targetPressure, writePressure);
    
    return controlData;
}

bool PressureController::writeToPLC(const QMap<QString, QVariant>& controlData) {
    if (!m_plcClient || !m_plcClient->isConnected()) {
        return false;
    }
    
    if (!controlData.contains("write_pressure") || !controlData.contains("control_address")) {
        return false;
    }
    
    QString address = controlData["control_address"].toString();
    double pressure = controlData["write_pressure"].toDouble();
    
    qInfo() << "Writing pressure" << pressure << "to" << address;
    
    bool success = m_plcClient->writeData(address, pressure);
    
    if (success) {
        m_lastWriteTime = QDateTime::currentMSecsSinceEpoch();
    }
    
    return success;
}

void PressureController::setEnabled(bool enabled) {
    m_enabled = enabled;
    qInfo() << "Pressure controller" << (enabled ? "enabled" : "disabled");
}

bool PressureController::isEnabled() const {
    return m_enabled;
}

void PressureController::setControlPressureAddress(const QString& address) {
    m_controlPressureAddress = address;
}

void PressureController::setEndPressureAddress(const QString& address) {
    m_endPressureAddress = address;
}

void PressureController::setFlowAddress(const QString& address) {
    m_flowAddress = address;
}

void PressureController::setDefaultPressure(double pressure) {
    m_defaultPressure = pressure;
}

void PressureController::setMinPressure(double pressure) {
    m_minPressure = pressure;
}

void PressureController::setMaxPressure(double pressure) {
    m_maxPressure = pressure;
}

void PressureController::setPressureIncreaseInterval(int seconds) {
    m_pressureIncreaseInterval = seconds;
}

double PressureController::calculateTargetPressure(const QMap<QString, QVariant>& data) {
    double endPressure = data.value("end_pressure", 0.0).toDouble();
    double currentFlow = data.value("flow", 0.0).toDouble();
    
    double yesterdayAvgFlow = getYesterdayAverageFlow();
    double day7AvgFlow = get7DayAverageFlow();
    
    double targetPressure = m_defaultPressure;
    
    if (endPressure > 0) {
        double pressureAdjust = 0.0;
        
        if (currentFlow > yesterdayAvgFlow * 1.2) {
            pressureAdjust = 0.05;
        } else if (currentFlow < yesterdayAvgFlow * 0.8) {
            pressureAdjust = -0.03;
        }
        
        targetPressure = endPressure + pressureAdjust;
    }
    
    targetPressure = qBound(m_minPressure, targetPressure, m_maxPressure);
    
    qInfo() << "Calculated target pressure:" << targetPressure 
            << "end:" << endPressure 
            << "flow:" << currentFlow 
            << "yesterday avg:" << yesterdayAvgFlow;
    
    return targetPressure;
}

double PressureController::calculateWritePressure(double targetPressure) {
    double writePressure = targetPressure;
    
    if (m_lastWritePressure > 0) {
        double maxChange = 0.05;
        double diff = targetPressure - m_lastWritePressure;
        
        if (qAbs(diff) > maxChange) {
            writePressure = m_lastWritePressure + (diff > 0 ? maxChange : -maxChange);
        }
    }
    
    writePressure = qBound(m_minPressure, writePressure, m_maxPressure);
    
    return writePressure;
}

double PressureController::getYesterdayAverageFlow() {
    QDateTime now = QDateTime::currentDateTime();
    QDateTime yesterdayStart = now.addDays(-1).date().startOfDay();
    QDateTime yesterdayEnd = yesterdayStart.addDays(1);
    
    QList<PLCDataRecord> records = DatabaseManager::instance()->getHistoryData(
        m_flowAddress, yesterdayStart, yesterdayEnd);
    
    if (records.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    int count = 0;
    
    for (const PLCDataRecord& record : records) {
        if (record.correctedValue.isValid()) {
            sum += record.correctedValue.toDouble();
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0.0;
}

double PressureController::get7DayAverageFlow() {
    QDateTime now = QDateTime::currentDateTime();
    QDateTime start = now.addDays(-7);
    
    QList<PLCDataRecord> records = DatabaseManager::instance()->getHistoryData(
        m_flowAddress, start, now);
    
    if (records.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    int count = 0;
    
    for (const PLCDataRecord& record : records) {
        if (record.correctedValue.isValid()) {
            sum += record.correctedValue.toDouble();
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0.0;
}
