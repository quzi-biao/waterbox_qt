#ifndef PLCCLIENT_H
#define PLCCLIENT_H

#include "interfaces/IPLCClient.h"
#include <QObject>
#include <QSharedPointer>

class PLCProtocol;

class PLCClient : public QObject, public IPLCClient {
    Q_OBJECT
    
public:
    explicit PLCClient(QObject* parent = nullptr);
    ~PLCClient();
    
    Q_INVOKABLE void setProtocol(const QString& protocolName);
    
    Q_INVOKABLE bool connect(const QString& host, int port) override;
    Q_INVOKABLE void disconnect() override;
    Q_INVOKABLE bool isConnected() const override;
    
    Q_INVOKABLE QVariant readData(const QString& address) override;
    Q_INVOKABLE QVariant readDataWithType(const QString& address, int dataType);
    Q_INVOKABLE bool writeData(const QString& address, const QVariant& value) override;
    
    Q_INVOKABLE QMap<QString, QVariant> readMultiple(const QStringList& addresses) override;
    Q_INVOKABLE bool writeMultiple(const QMap<QString, QVariant>& data) override;
    
signals:
    void connected();
    void disconnected();
    void error(const QString& message);
    
private:
    QSharedPointer<PLCProtocol> m_protocol;
    QString m_host;
    int m_port;
    bool m_connected;
};

class PLCProtocol {
public:
    virtual ~PLCProtocol() = default;
    
    virtual bool connect(const QString& host, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    
    virtual QVariant read(const QString& address) = 0;
    virtual bool write(const QString& address, const QVariant& value) = 0;
};

#endif
