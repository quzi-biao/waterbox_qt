#ifndef DATASENDER_H
#define DATASENDER_H

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <QJsonObject>

class IDataReporter;
class DatabaseManager;
class HttpSender;

class DataSender : public QObject {
    Q_OBJECT
    
public:
    explicit DataSender(HttpSender* sender, QObject* parent = nullptr);
    
    void setDataCollector(class DataCollector* collector) { m_collector = collector; }
    
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void setInterval(int milliseconds);
    
    Q_INVOKABLE void addData(const QJsonObject& data);
    
signals:
    void dataSent(qint64 recordId);
    void sendError(const QString& error);
    
private slots:
    void sendPendingData();
    void adjustSendFrequency();
    
private:
    HttpSender* m_sender;
    class DataCollector* m_collector;
    QTimer* m_sendTimer;
    QTimer* m_adjustTimer;
    
    int m_baseInterval;
    int m_minInterval;
    int m_currentInterval;
    
    bool m_started;
};

#endif
