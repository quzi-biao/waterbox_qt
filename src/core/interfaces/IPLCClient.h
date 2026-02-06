#ifndef IPLCCLIENT_H
#define IPLCCLIENT_H

#include <QString>
#include <QVariant>
#include <QMap>

class IPLCClient {
public:
    virtual ~IPLCClient() = default;
    
    virtual bool connect(const QString& host, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    virtual QVariant readData(const QString& address) = 0;
    virtual bool writeData(const QString& address, const QVariant& value) = 0;
    
    virtual QMap<QString, QVariant> readMultiple(const QStringList& addresses) = 0;
    virtual bool writeMultiple(const QMap<QString, QVariant>& data) = 0;
};

#endif
