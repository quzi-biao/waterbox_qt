#ifndef PLCSIMULATOR_H
#define PLCSIMULATOR_H

#include "interfaces/IPLCClient.h"
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QRandomGenerator>

class PLCSimulator : public QObject, public IPLCClient {
    Q_OBJECT
    
public:
    explicit PLCSimulator(QObject* parent = nullptr);
    
    bool connect(const QString& host, int port) override;
    void disconnect() override;
    bool isConnected() const override;
    
    QVariant readData(const QString& address) override;
    bool writeData(const QString& address, const QVariant& value) override;
    
    QMap<QString, QVariant> readMultiple(const QStringList& addresses) override;
    bool writeMultiple(const QMap<QString, QVariant>& data) override;
    
    void setSimulationMode(bool enabled);
    void setUpdateInterval(int milliseconds);
    
signals:
    void dataUpdated();
    
private slots:
    void updateSimulatedData();
    
private:
    double generateFlowValue();
    double generatePressureValue();
    
    bool m_connected;
    bool m_simulationEnabled;
    QTimer* m_updateTimer;
    QMap<QString, QVariant> m_simulatedData;
    QRandomGenerator* m_random;
    
    double m_baseFlow;
    double m_basePressure;
    qint64 m_startTime;
};

#endif
