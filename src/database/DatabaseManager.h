#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QVariant>
#include <QDateTime>
#include <QMutex>

struct PLCDataRecord {
    qint64 id;
    QDateTime timestamp;
    QString address;
    QVariant rawValue;
    QVariant correctedValue;
    bool uploaded;
    bool isStressTest;
    
    PLCDataRecord() : id(0), uploaded(false), isStressTest(false) {}
};

class DatabaseManager : public QObject {
    Q_OBJECT
    
public:
    static DatabaseManager* instance();
    
    bool initialize();
    void close();
    
    bool saveData(const QString& address, const QVariant& rawValue, const QVariant& correctedValue, bool isStressTest = false);
    bool markAsUploaded(qint64 id);
    
    QList<PLCDataRecord> getUnuploadedData(int limit = 100);
    QList<PLCDataRecord> getHistoryData(const QString& address, const QDateTime& start, const QDateTime& end);
    QMap<QString, QVariant> getLatestData();  // 获取每个地址的最新数据
    
    bool saveKeyValue(const QString& key, const QVariant& value);
    QVariant getKeyValue(const QString& key, const QVariant& defaultValue = QVariant());
    
    QList<class MetricIndicator> loadMetricIndicators();
    
    void cleanOldData(int daysToKeep = 365);
    int deleteStressTestData();
    int getStressTestDataCount();
    
private:
    DatabaseManager();
    ~DatabaseManager();
    
    static DatabaseManager* m_instance;
    QSqlDatabase m_db;
    QMutex m_mutex;  // 保护数据库访问的互斥锁
    
    bool createTables();
};

#endif
