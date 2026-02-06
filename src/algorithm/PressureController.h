#ifndef PRESSURECONTROLLER_H
#define PRESSURECONTROLLER_H

#include "interfaces/ISmartController.h"
#include <QObject>

class IPLCClient;
class DatabaseManager;

class PressureController : public QObject, public ISmartController {
    Q_OBJECT
    
public:
    explicit PressureController(IPLCClient* plcClient, QObject* parent = nullptr);
    
    QMap<QString, QVariant> readData() override;
    QMap<QString, QVariant> processData(const QMap<QString, QVariant>& inputData) override;
    bool writeToPLC(const QMap<QString, QVariant>& controlData) override;
    
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;
    
    void setControlPressureAddress(const QString& address);
    void setEndPressureAddress(const QString& address);
    void setFlowAddress(const QString& address);
    
    void setDefaultPressure(double pressure);
    void setMinPressure(double pressure);
    void setMaxPressure(double pressure);
    void setPressureIncreaseInterval(int seconds);
    
signals:
    void pressureCalculated(double targetPressure, double writePressure);
    
private:
    double calculateTargetPressure(const QMap<QString, QVariant>& data);
    double calculateWritePressure(double targetPressure);
    double getYesterdayAverageFlow();
    double get7DayAverageFlow();
    
    IPLCClient* m_plcClient;
    bool m_enabled;
    
    QString m_controlPressureAddress;
    QString m_endPressureAddress;
    QString m_flowAddress;
    
    double m_defaultPressure;
    double m_minPressure;
    double m_maxPressure;
    int m_pressureIncreaseInterval;
    
    qint64 m_lastWriteTime;
    double m_lastWritePressure;
    double m_lastTargetPressure;
};

#endif
