#ifndef REMOTECONFIGRECEIVER_H
#define REMOTECONFIGRECEIVER_H

#include <QObject>
#include <QJsonObject>

class RemoteConfigReceiver : public QObject {
    Q_OBJECT
    
public:
    explicit RemoteConfigReceiver(QObject* parent = nullptr);
    
    void processResponse(const QString& response);
    
signals:
    void configReceived(const QJsonObject& config);
    void plcWriteCommand(const QString& address, const QVariant& value);
    
private:
    void handleCommand(const QJsonObject& cmd);
};

#endif
