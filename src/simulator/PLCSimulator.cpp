#include "PLCSimulator.h"
#include <QDateTime>
#include <QtMath>
#include <QDebug>

PLCSimulator::PLCSimulator(QObject* parent)
    : QObject(parent),
      m_connected(false),
      m_simulationEnabled(false),
      m_baseFlow(100.0),
      m_basePressure(0.35),
      m_startTime(0) {
    
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    QObject::connect(m_updateTimer, &QTimer::timeout, this, &PLCSimulator::updateSimulatedData);
    
    m_random = QRandomGenerator::global();
}

bool PLCSimulator::connect(const QString& host, int port) {
    Q_UNUSED(host);
    Q_UNUSED(port);
    
    m_connected = true;
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    
    if (m_simulationEnabled) {
        m_updateTimer->start();
    }
    
    qInfo() << "PLC Simulator connected";
    return true;
}

void PLCSimulator::disconnect() {
    m_connected = false;
    m_updateTimer->stop();
    qInfo() << "PLC Simulator disconnected";
}

bool PLCSimulator::isConnected() const {
    return m_connected;
}

QVariant PLCSimulator::readData(const QString& address) {
    if (!m_connected) {
        return QVariant();
    }
    
    if (m_simulatedData.contains(address)) {
        return m_simulatedData[address];
    }
    
    if (address.contains("flow", Qt::CaseInsensitive) || 
        address.contains("V300")) {
        return generateFlowValue();
    } else if (address.contains("press", Qt::CaseInsensitive) || 
               address.contains("V200") || address.contains("V100")) {
        return generatePressureValue();
    }
    
    return m_random->bounded(100.0);
}

bool PLCSimulator::writeData(const QString& address, const QVariant& value) {
    if (!m_connected) {
        return false;
    }
    
    m_simulatedData[address] = value;
    qInfo() << "Simulator: Written" << value << "to" << address;
    return true;
}

QMap<QString, QVariant> PLCSimulator::readMultiple(const QStringList& addresses) {
    QMap<QString, QVariant> results;
    
    for (const QString& address : addresses) {
        results[address] = readData(address);
    }
    
    return results;
}

bool PLCSimulator::writeMultiple(const QMap<QString, QVariant>& data) {
    for (auto it = data.begin(); it != data.end(); ++it) {
        writeData(it.key(), it.value());
    }
    return true;
}

void PLCSimulator::setSimulationMode(bool enabled) {
    m_simulationEnabled = enabled;
    
    if (enabled && m_connected) {
        m_updateTimer->start();
        qInfo() << "Simulation mode enabled";
    } else {
        m_updateTimer->stop();
        qInfo() << "Simulation mode disabled";
    }
}

void PLCSimulator::setUpdateInterval(int milliseconds) {
    m_updateTimer->setInterval(milliseconds);
    qInfo() << "Simulation update interval set to" << milliseconds << "ms";
}

void PLCSimulator::updateSimulatedData() {
    m_simulatedData["V300"] = generateFlowValue();
    m_simulatedData["V200"] = generatePressureValue();
    m_simulatedData["V100"] = m_simulatedData.value("V100", m_basePressure);
    
    emit dataUpdated();
}

double PLCSimulator::generateFlowValue() {
    qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTime;
    double seconds = elapsed / 1000.0;
    
    QTime currentTime = QTime::currentTime();
    int hour = currentTime.hour();
    
    double hourFactor = 1.0;
    if (hour >= 6 && hour < 9) {
        hourFactor = 1.5;
    } else if (hour >= 11 && hour < 13) {
        hourFactor = 1.3;
    } else if (hour >= 18 && hour < 21) {
        hourFactor = 1.4;
    } else if (hour >= 0 && hour < 5) {
        hourFactor = 0.5;
    }
    
    double sinWave = qSin(seconds / 60.0 * 2 * M_PI) * 10;
    double noise = (m_random->bounded(100) - 50) / 10.0;
    
    double flow = m_baseFlow * hourFactor + sinWave + noise;
    return qMax(0.0, flow);
}

double PLCSimulator::generatePressureValue() {
    qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTime;
    double seconds = elapsed / 1000.0;
    
    double sinWave = qSin(seconds / 30.0 * 2 * M_PI) * 0.02;
    double noise = (m_random->bounded(100) - 50) / 1000.0;
    
    double pressure = m_basePressure + sinWave + noise;
    return qBound(0.2, pressure, 0.5);
}
