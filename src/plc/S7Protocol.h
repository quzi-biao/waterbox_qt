#ifndef S7PROTOCOL_H
#define S7PROTOCOL_H

#include "PLCClient.h"

class S7Protocol : public PLCProtocol {
public:
    S7Protocol();
    ~S7Protocol();
    
    bool connect(const QString& host, int port) override;
    void disconnect() override;
    bool isConnected() const override;
    
    QVariant read(const QString& address) override;
    QVariant readWithType(const QString& address, int dataType);
    bool write(const QString& address, const QVariant& value) override;
    
private:
    class Snap7Simple* m_snap7;
    
    struct S7Address {
        int db;
        int offset;
        int bit;
        char type;
        
        S7Address() : db(0), offset(0), bit(-1), type('X') {}
    };
    
    S7Address parseAddress(const QString& address);
};

#endif
