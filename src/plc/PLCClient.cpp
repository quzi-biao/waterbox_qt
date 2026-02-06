#include "PLCClient.h"
#include "S7Protocol.h"
#include "ModbusProtocol.h"
#include <QDebug>

PLCClient::PLCClient(QObject* parent)
    : QObject(parent), m_port(0), m_connected(false) {
}

PLCClient::~PLCClient() {
    disconnect();
}

void PLCClient::setProtocol(const QString& protocolName) {
    if (protocolName.toUpper() == "S7") {
        m_protocol = QSharedPointer<PLCProtocol>(new S7Protocol());
    } else if (protocolName.toUpper() == "MODBUS") {
        m_protocol = QSharedPointer<PLCProtocol>(new ModbusProtocol());
    } else {
        qWarning() << "Unknown protocol:" << protocolName;
    }
}

bool PLCClient::connect(const QString& host, int port) {
    if (!m_protocol) {
        emit error("Protocol not set");
        return false;
    }
    
    m_host = host;
    m_port = port;
    
    if (m_protocol->connect(host, port)) {
        m_connected = true;
        emit connected();
        qInfo() << "Connected to PLC at" << host << ":" << port;
        return true;
    }
    
    emit error("Failed to connect to PLC");
    return false;
}

void PLCClient::disconnect() {
    if (m_protocol && m_connected) {
        m_protocol->disconnect();
        m_connected = false;
        emit disconnected();
        qInfo() << "Disconnected from PLC";
    }
}

bool PLCClient::isConnected() const {
    return m_connected && m_protocol && m_protocol->isConnected();
}

QVariant PLCClient::readData(const QString& address) {
    if (!isConnected()) {
        qWarning() << "Not connected to PLC";
        return QVariant();
    }
    
    return m_protocol->read(address);
}

QVariant PLCClient::readDataWithType(const QString& address, int dataType) {
    if (!isConnected()) {
        qWarning() << "Not connected to PLC";
        return QVariant();
    }
    
    S7Protocol* s7 = dynamic_cast<S7Protocol*>(m_protocol.data());
    if (s7) {
        return s7->readWithType(address, dataType);
    }
    
    return m_protocol->read(address);
}

bool PLCClient::writeData(const QString& address, const QVariant& value) {
    if (!isConnected()) {
        qWarning() << "Not connected to PLC";
        return false;
    }
    
    return m_protocol->write(address, value);
}

QMap<QString, QVariant> PLCClient::readMultiple(const QStringList& addresses) {
    QMap<QString, QVariant> results;
    
    for (const QString& address : addresses) {
        QVariant value = readData(address);
        if (value.isValid()) {
            results[address] = value;
        }
    }
    
    return results;
}

bool PLCClient::writeMultiple(const QMap<QString, QVariant>& data) {
    bool allSuccess = true;
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (!writeData(it.key(), it.value())) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}
