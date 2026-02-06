#ifndef DATACOLLECTOR_H
#define DATACOLLECTOR_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QVariant>
#include <QList>

class IPLCClient;
class MetricIndicator;
class PLCClient;
class DatabaseManager;

class DataCollector : public QObject {
    Q_OBJECT
    
public:
    explicit DataCollector(IPLCClient* plcClient, QObject* parent = nullptr);
    ~DataCollector();
    
    Q_INVOKABLE void startCollection();
    Q_INVOKABLE void stopCollection();
    Q_INVOKABLE void setInterval(int milliseconds);
    
    Q_INVOKABLE void setDataSchema(const QMap<QString, QString>& schema);
    QList<MetricIndicator> getMetricIndicators() const;
    Q_INVOKABLE QMap<QString, QVariant> getLatestData() const;
    
signals:
    void dataCollected(const QMap<QString, QVariant>& data);
    void collectionError(const QString& error);
    
private slots:
    void collectData();
    
private:
    QVariant readIndicatorValue(const QString& address, int dataType);
    QVariant correctValue(const QString& address, const QVariant& rawValue);
    QVariant fillMissingValue(const QString& address);
    
    IPLCClient* m_plcClient;
    QTimer* m_timer;
    QMap<QString, QString> m_dataSchema;
    QMap<QString, QVariant> m_latestData;
    QMap<QString, QVariant> m_lastValidData;
};

#endif
