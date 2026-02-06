#ifndef SNAP7_SIMPLE_H
#define SNAP7_SIMPLE_H

#include <QString>
#include <QTcpSocket>
#include <QByteArray>

// 简化的 S7 客户端，专门用于 S200 SMART
class Snap7Simple {
public:
    Snap7Simple();
    ~Snap7Simple();
    
    bool connect(const QString& host, int port);
    void disconnect();
    bool isConnected() const;
    
    // 读取不同类型的数据
    bool readBytes(int dbNumber, int offset, int length, unsigned char* buffer);
    bool readBit(int dbNumber, int offset, int bit);
    unsigned char readByte(int dbNumber, int offset);
    float readFloat(int dbNumber, int offset);
    int readInt32(int dbNumber, int offset);
    short readInt16(int dbNumber, int offset);
    
private:
    QTcpSocket* m_socket;
    bool m_connected;
    
    // S7 协议辅助方法
    bool sendCOTPConnect();
    bool sendCOTPConnectWithTSAP(quint8 srcHigh, quint8 srcLow, quint8 dstHigh, quint8 dstLow);
    bool sendS7Setup();
    QByteArray buildReadRequest(int dbNumber, int offset, int length);
    bool sendAndReceive(const QByteArray& request, QByteArray& response);
};

#endif
