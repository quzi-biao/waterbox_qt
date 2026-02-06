#ifndef MODBUSPROTOCOL_H
#define MODBUSPROTOCOL_H

#include "PLCClient.h"
#include <QTcpSocket>

class ModbusProtocol : public PLCProtocol {
public:
    ModbusProtocol();
    ~ModbusProtocol();
    
    bool connect(const QString& host, int port) override;
    void disconnect() override;
    bool isConnected() const override;
    
    QVariant read(const QString& address) override;
    bool write(const QString& address, const QVariant& value) override;
    
private:
    QTcpSocket* m_socket;
    quint16 m_transactionId;
    
    struct ModbusAddress {
        int registerAddress;
        int count;
        
        ModbusAddress() : registerAddress(0), count(1) {}
    };
    
    ModbusAddress parseAddress(const QString& address);
    QByteArray buildReadRequest(int address, int count);
    QByteArray buildWriteRequest(int address, quint16 value);
    bool sendAndReceive(const QByteArray& request, QByteArray& response);
};

#endif
