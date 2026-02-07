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
    
    void reloadConfig();
    
signals:
    void pressureCalculated(double targetPressure, double writePressure);
    
private:
    double getDstPress(double currentFlow);
    double calDstPress(double currentFlow);
    double calWritePress(double dstPress);
    double nearAvgFlow();
    
    IPLCClient* m_plcClient;
    bool m_enabled;
    
    // 配置项（从 ConfigManager 读取）
    QString m_ctrlPressPlcAddress;
    QString m_flowPlcAddress;          // 当前流量数据地址
    int m_pressCtrlType;
    double m_defaultPressure;
    double m_minPressure;
    double m_maxPressure;
    double m_pressIncreaseStep;
    int m_pressIncreaseInterval;
    
    // 压力计算公式参数
    double m_hst;   // 建筑高差
    double m_h0;    // 最不利点出流高差
    double m_rate;  // 水头损失
    double m_qs;    // 设计秒流量
    
    // 运行状态
    qint64 m_lastWriteTime;
    double m_lastCtrlPress;
    double m_lastWritePress;
};

#endif
