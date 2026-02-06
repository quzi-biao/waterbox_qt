#ifndef IDATAREPORTER_H
#define IDATAREPORTER_H

#include <QString>
#include <QJsonObject>

class IDataReporter {
public:
    virtual ~IDataReporter() = default;
    
    virtual bool establishConnection(const QString& url) = 0;
    virtual bool reportData(const QJsonObject& data) = 0;
    virtual void closeConnection() = 0;
    virtual bool isConnected() const = 0;
    
    virtual QString getLastResponse() const = 0;
};

#endif
