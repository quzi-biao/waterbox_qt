#include "S7Protocol.h"
#include "snap7_simple.h"
#include <QDebug>
#include <QRegularExpression>

S7Protocol::S7Protocol() : m_snap7(new Snap7Simple()) {
}

S7Protocol::~S7Protocol() {
    disconnect();
    delete m_snap7;
}

bool S7Protocol::connect(const QString& host, int port) {
    return m_snap7->connect(host, port);
}

void S7Protocol::disconnect() {
    m_snap7->disconnect();
}

bool S7Protocol::isConnected() const {
    return m_snap7 && m_snap7->isConnected();
}

S7Protocol::S7Address S7Protocol::parseAddress(const QString& address) {
    S7Address addr;
    
    // 处理 VD、VW 格式：VD10 -> V10, VW20 -> V20
    QString processedAddress = address;
    processedAddress = processedAddress.replace("VD", "V");
    processedAddress = processedAddress.replace("VW", "V");
    processedAddress = processedAddress.replace("MD", "M");
    processedAddress = processedAddress.replace("MW", "M");
    
    // 支持位地址：M10.0, V5.1 等
    QRegularExpression reBit("([A-Z]+)(\\d+)\\.(\\d+)");
    QRegularExpressionMatch matchBit = reBit.match(processedAddress);
    
    if (matchBit.hasMatch()) {
        // 位地址格式：M10.0
        QString area = matchBit.captured(1);
        int offset = matchBit.captured(2).toInt();
        int bit = matchBit.captured(3).toInt();
        
        addr.type = area[0].toLatin1();
        addr.offset = offset;
        addr.bit = bit;
        addr.db = 1;
        return addr;
    }
    
    // 普通地址格式：V10, M10
    QRegularExpression re("([A-Z]+)(\\d+)");
    QRegularExpressionMatch match = re.match(processedAddress);
    
    if (match.hasMatch()) {
        QString area = match.captured(1);
        int offset = match.captured(2).toInt();
        
        addr.type = area[0].toLatin1();
        addr.offset = offset;
        addr.db = 1;
    }
    
    return addr;
}

QVariant S7Protocol::read(const QString& address) {
    // 默认使用 INT16 类型读取
    return readWithType(address, 0);
}

QVariant S7Protocol::readWithType(const QString& address, int dataType) {
    S7Address addr = parseAddress(address);
    
    // 使用 Snap7Simple 读取
    if (m_snap7->isConnected()) {
        switch (dataType) {
            case 0: // INT16
            case 2: // UINT16
                return m_snap7->readInt16(1, addr.offset);
            case 1: // INT32
            case 3: // UINT32
                return m_snap7->readInt32(1, addr.offset);
            case 4: // FLOAT32
                return m_snap7->readFloat(1, addr.offset);
            case 5: // FLOAT64
                return QVariant();
            case 6: // BYTE
                return (int)m_snap7->readByte(1, addr.offset);
            case 7: // BOOL
                if (addr.bit >= 0) {
                    return m_snap7->readBit(1, addr.offset, addr.bit);
                }
                return QVariant();
            default:
                return QVariant();
        }
    }
    
    return QVariant();
}

bool S7Protocol::write(const QString& address, const QVariant& value) {
    // TODO: 实现写入功能
    Q_UNUSED(address);
    Q_UNUSED(value);
    return false;
}

