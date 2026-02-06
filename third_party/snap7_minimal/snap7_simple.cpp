#include "snap7_simple.h"
#include <QDebug>
#include <QtEndian>

Snap7Simple::Snap7Simple() 
    : m_socket(new QTcpSocket()), m_connected(false) {
}

Snap7Simple::~Snap7Simple() {
    disconnect();
    delete m_socket;
}

bool Snap7Simple::connect(const QString& host, int port) {
    m_socket->connectToHost(host, port);
    
    if (!m_socket->waitForConnected(5000)) {
        qWarning() << "Snap7Simple: TCP 连接失败";
        return false;
    }
    
    qInfo() << "Snap7Simple: TCP 连接成功";
    
    // S200 SMART 可能需要特定的 TSAP 组合
    // 尝试多种常见的 TSAP 配置
    struct TSAPConfig {
        quint8 srcHigh, srcLow, dstHigh, dstLow;
        const char* name;
    };
    
    TSAPConfig configs[] = {
        {0x10, 0x00, 0x03, 0x00, "0x1000-0x0300"},  // S200 SMART 正确配置！
        {0x10, 0x00, 0x02, 0x00, "0x1000-0x0200"},  // S200 SMART 备选1
        {0x10, 0x00, 0x02, 0x01, "0x1000-0x0201"},  // S200 SMART 备选2
        {0x10, 0x00, 0x03, 0x01, "0x1000-0x0301"},  // S200 SMART 备选3
        {0x01, 0x00, 0x01, 0x00, "0x0100-0x0100"},  // S1200/S1500
    };
    
    for (const auto& config : configs) {
        if (sendCOTPConnectWithTSAP(config.srcHigh, config.srcLow, config.dstHigh, config.dstLow)) {
            qInfo() << "S7: COTP 握手成功";
            
            // 发送 S7 通信设置
            if (sendS7Setup()) {
                m_connected = true;
                qInfo() << "Snap7Simple: 连接完全建立";
                return true;
            }
        }
    }
    
    qWarning() << "Snap7Simple: 所有 TSAP 配置都失败";
    return false;
}

void Snap7Simple::disconnect() {
    if (m_socket->isOpen()) {
        m_socket->close();
    }
    m_connected = false;
}

bool Snap7Simple::isConnected() const {
    return m_connected && m_socket->state() == QAbstractSocket::ConnectedState;
}

bool Snap7Simple::sendCOTPConnectWithTSAP(quint8 srcHigh, quint8 srcLow, quint8 dstHigh, quint8 dstLow) {
    QByteArray request;
    request.append((char)0x03); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x16);
    request.append((char)0x11); request.append((char)0xE0);
    request.append((char)0x00); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x01);
    request.append((char)0x00); request.append((char)0xC0);
    request.append((char)0x01); request.append((char)0x0A);
    request.append((char)0xC1); request.append((char)0x02);
    request.append((char)srcHigh); request.append((char)srcLow);
    request.append((char)0xC2); request.append((char)0x02);
    request.append((char)dstHigh); request.append((char)dstLow);
    
    m_socket->write(request);
    m_socket->waitForBytesWritten(1000);
    
    // 尝试读取响应
    QByteArray response;
    for (int i = 0; i < 3; i++) {
        if (m_socket->waitForReadyRead(1000)) {
            response.append(m_socket->readAll());
            if (response.size() >= 7) break;
        }
    }
    
    if (!response.isEmpty() && response.size() >= 6 && (quint8)response.at(5) == 0xD0) {
        return true;
    }
    
    return false;
}

bool Snap7Simple::sendCOTPConnect() {
    // S200 SMART 特殊的 COTP 连接请求
    // 使用 TSAP: 0x0100 (而不是 0x1000)
    QByteArray request;
    request.append((char)0x03); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x16);
    request.append((char)0x11); request.append((char)0xE0);
    request.append((char)0x00); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x01);
    request.append((char)0x00); request.append((char)0xC0);
    request.append((char)0x01); request.append((char)0x0A);
    request.append((char)0xC1); request.append((char)0x02);
    request.append((char)0x01); request.append((char)0x00);  // src-tsap: 0x0100
    request.append((char)0xC2); request.append((char)0x02);
    request.append((char)0x01); request.append((char)0x00);  // dst-tsap: 0x0100
    
    // 输出请求的十六进制
    QString hex;
    for (int i = 0; i < request.size(); i++) {
        hex += QString("%1 ").arg((quint8)request.at(i), 2, 16, QChar('0')).toUpper();
    }
    qDebug() << "Snap7Simple COTP 请求:" << hex;
    
    m_socket->write(request);
    m_socket->waitForBytesWritten(1000);
    
    // 尝试多次读取
    QByteArray response;
    for (int i = 0; i < 3; i++) {
        if (m_socket->waitForReadyRead(1000)) {
            response.append(m_socket->readAll());
            if (response.size() >= 7) break;
        }
    }
    
    if (!response.isEmpty()) {
        QString respHex;
        for (int i = 0; i < response.size(); i++) {
            respHex += QString("%1 ").arg((quint8)response.at(i), 2, 16, QChar('0')).toUpper();
        }
        qDebug() << "Snap7Simple COTP 响应:" << respHex;
    }
    
    return response.size() >= 7;
}

bool Snap7Simple::sendS7Setup() {
    QByteArray request;
    request.append((char)0x03); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x19);
    request.append((char)0x02); request.append((char)0xF0);
    request.append((char)0x80); request.append((char)0x32);
    request.append((char)0x01); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x00);
    request.append((char)0x08); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0xF0);
    request.append((char)0x00); request.append((char)0x00);
    request.append((char)0x01); request.append((char)0x00);
    request.append((char)0x01); request.append((char)0x03);
    request.append((char)0xC0);
    
    m_socket->write(request);
    m_socket->waitForBytesWritten(1000);
    
    if (!m_socket->waitForReadyRead(2000)) {
        return false;
    }
    
    QByteArray response = m_socket->readAll();
    return response.size() >= 20;
}

QByteArray Snap7Simple::buildReadRequest(int dbNumber, int offset, int length) {
    QByteArray request;
    
    // TPKT + COTP
    request.append((char)0x03); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x1F);
    request.append((char)0x02); request.append((char)0xF0);
    request.append((char)0x80);
    
    // S7 Header
    request.append((char)0x32); request.append((char)0x01);
    request.append((char)0x00); request.append((char)0x00);
    request.append((char)0x00); request.append((char)0x01);
    request.append((char)0x00); request.append((char)0x0E);
    request.append((char)0x00); request.append((char)0x00);
    
    // S7 Parameter
    request.append((char)0x04); request.append((char)0x01);
    request.append((char)0x12); request.append((char)0x0A);
    request.append((char)0x10); request.append((char)0x02);
    request.append((char)((length >> 8) & 0xFF));
    request.append((char)(length & 0xFF));
    request.append((char)((dbNumber >> 8) & 0xFF));
    request.append((char)(dbNumber & 0xFF));
    request.append((char)0x84);
    
    int addressBits = offset * 8;
    request.append((char)((addressBits >> 16) & 0xFF));
    request.append((char)((addressBits >> 8) & 0xFF));
    request.append((char)(addressBits & 0xFF));
    
    return request;
}

bool Snap7Simple::sendAndReceive(const QByteArray& request, QByteArray& response) {
    if (!isConnected()) return false;
    
    m_socket->write(request);
    m_socket->waitForBytesWritten(1000);
    
    if (!m_socket->waitForReadyRead(2000)) {
        return false;
    }
    
    response = m_socket->readAll();
    return !response.isEmpty();
}

bool Snap7Simple::readBytes(int dbNumber, int offset, int length, unsigned char* buffer) {
    QByteArray request = buildReadRequest(dbNumber, offset, length);
    QByteArray response;
    
    if (!sendAndReceive(request, response)) {
        return false;
    }
    
    // S7 响应格式：数据在第 25 字节开始
    // TPKT(4) + COTP(3) + S7Header(12) + S7Param(2) + S7Data(4+数据)
    if (response.size() < 25 + length) {
        return false;
    }
    
    // 数据从第 25 字节开始
    memcpy(buffer, response.constData() + 25, length);
    
    return true;
}

bool Snap7Simple::readBit(int dbNumber, int offset, int bit) {
    unsigned char buffer[1];
    if (!readBytes(dbNumber, offset, 1, buffer)) {
        return false;
    }
    
    return (buffer[0] & (1 << bit)) != 0;
}

unsigned char Snap7Simple::readByte(int dbNumber, int offset) {
    unsigned char buffer[1];
    if (!readBytes(dbNumber, offset, 1, buffer)) {
        return 0;
    }
    
    return buffer[0];
}

float Snap7Simple::readFloat(int dbNumber, int offset) {
    unsigned char buffer[4];
    if (!readBytes(dbNumber, offset, 4, buffer)) {
        return 0.0f;
    }
    
    quint32 value = qFromBigEndian<quint32>(buffer);
    return *reinterpret_cast<float*>(&value);
}

int Snap7Simple::readInt32(int dbNumber, int offset) {
    unsigned char buffer[4];
    if (!readBytes(dbNumber, offset, 4, buffer)) {
        return 0;
    }
    
    return qFromBigEndian<qint32>(buffer);
}

short Snap7Simple::readInt16(int dbNumber, int offset) {
    unsigned char buffer[2];
    if (!readBytes(dbNumber, offset, 2, buffer)) {
        return 0;
    }
    
    return qFromBigEndian<qint16>(buffer);
}
