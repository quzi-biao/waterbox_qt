#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>

class HttpSender;

struct SystemMonitorData {
    qint64 collectTimestamp;
    double cpu;
    double mem;
    double disk;
    
    SystemMonitorData() : collectTimestamp(0), cpu(0.0), mem(0.0), disk(0.0) {}
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["collectTimestamp"] = collectTimestamp;
        obj["cpu"] = cpu;
        obj["mem"] = mem;
        obj["disk"] = disk;
        return obj;
    }
};

class SystemMonitor : public QObject {
    Q_OBJECT
    
public:
    explicit SystemMonitor(QObject* parent = nullptr);
    ~SystemMonitor();
    
    void initialize(HttpSender* sender);
    void start();
    void stop();
    
    SystemMonitorData getCurrentData() const;
    
private slots:
    void collectAndReport();
    
private:
    void getSystemInfo();
    double getCpuUsage();
    double getMemoryUsage();
    double getDiskUsage();
    
    HttpSender* m_sender;
    QTimer* m_timer;
    
    SystemMonitorData m_currentData;
    
    qint64 m_updateInterval;
    
    static const QString HARDWARE_MONITOR_UPDATE_TIME;
    static const QString MAX_MEMORY_SIZE;
    static const QString MAX_DISK_SIZE;
};

#endif
