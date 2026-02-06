#include "ModbusProtocol.h"
#include <QDebug>

ModbusProtocol::ModbusProtocol() 
    : m_socket(new QTcpSocket()), m_transactionId(0) {
}

ModbusProtocol::~ModbusProtocol() {
    disconnect();
    delete m_socket;
}

bool ModbusProtocol::connect(const QString& host, int port) {
    qInfo() << "Modbus: 尝试连接到" << host << ":" << port;
    m_socket->connectToHost(host, port);
    
    if (m_socket->waitForConnected(5000)) {
        qInfo() << "Modbus: TCP 连接成功！";
        return true;
    } else {
        qWarning() << "Modbus: 连接失败:" << m_socket->errorString();
        return false;
    }
}

void ModbusProtocol::disconnect() {
    if (m_socket->isOpen()) {
        m_socket->close();
    }
}

bool ModbusProtocol::isConnected() const {
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

ModbusProtocol::ModbusAddress ModbusProtocol::parseAddress(const QString& address) {
    ModbusAddress addr;
    addr.registerAddress = address.toInt();
    return addr;
}

QVariant ModbusProtocol::read(const QString& address) {
    ModbusAddress addr = parseAddress(address);
    
    QByteArray request = buildReadRequest(addr.registerAddress, 1);
    QByteArray response;
    
    if (!sendAndReceive(request, response)) {
        return QVariant();
    }
    
    if (response.size() < 11) {
        return QVariant();
    }
    
    quint16 value = (static_cast<quint8>(response.at(9)) << 8) | 
                    static_cast<quint8>(response.at(10));
    return value;
}

bool ModbusProtocol::write(const QString& address, const QVariant& value) {
    ModbusAddress addr = parseAddress(address);
    
    QByteArray request = buildWriteRequest(addr.registerAddress, value.toUInt());
    QByteArray response;
    
    return sendAndReceive(request, response);
}

QByteArray ModbusProtocol::buildReadRequest(int address, int count) {
    QByteArray request;
    
    m_transactionId++;
    request.append((char)((m_transactionId >> 8) & 0xFF));
    request.append((char)(m_transactionId & 0xFF));
    request.append((char)0x00);
    request.append((char)0x00);
    request.append((char)0x00);
    request.append((char)0x06);
    request.append((char)0x01);
    request.append((char)0x03);
    request.append((char)((address >> 8) & 0xFF));
    request.append((char)(address & 0xFF));
    request.append((char)((count >> 8) & 0xFF));
    request.append((char)(count & 0xFF));
    
    return request;
}

QByteArray ModbusProtocol::buildWriteRequest(int address, quint16 value) {
    QByteArray request;
    
    m_transactionId++;
    request.append((char)((m_transactionId >> 8) & 0xFF));
    request.append((char)(m_transactionId & 0xFF));
    request.append((char)0x00);
    request.append((char)0x00);
    request.append((char)0x00);
    request.append((char)0x06);
    request.append((char)0x01);
    request.append((char)0x06);
    request.append((char)((address >> 8) & 0xFF));
    request.append((char)(address & 0xFF));
    request.append((char)((value >> 8) & 0xFF));
    request.append((char)(value & 0xFF));
    
    return request;
}

bool ModbusProtocol::sendAndReceive(const QByteArray& request, QByteArray& response) {
    if (!isConnected()) {
        return false;
    }
    
    m_socket->write(request);
    if (!m_socket->waitForBytesWritten(1000)) {
        return false;
    }
    
    if (!m_socket->waitForReadyRead(2000)) {
        return false;
    }
    
    response = m_socket->readAll();
    return !response.isEmpty();
}
