#ifndef SYSTEMCONFIGWIDGET_H
#define SYSTEMCONFIGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

class SystemConfigWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit SystemConfigWidget(QWidget* parent = nullptr);
    
    void loadConfig();
    void saveConfig();
    
signals:
    void configSaved();
    void stressTestModeChanged(bool enabled);
    void cleanStressTestData();
    
private:
    void setupUI();
    
    // 云端配置
    QLineEdit* m_serviceAddressEdit;
    
    // PLC 配置
    QCheckBox* m_plcServerCheck;
    QLineEdit* m_plcLocalHostEdit;
    QSpinBox* m_plcPortSpin;
    QComboBox* m_plcProtocolCombo;
    QSpinBox* m_dataCollectIntervalSpin;            // 数据采集间隔(ms)
    
    // 水箱参数
    QDoubleSpinBox* m_boxLongSpin;
    QDoubleSpinBox* m_boxWideSpin;
    QDoubleSpinBox* m_boxHighSpin;
    QSpinBox* m_boxNumSpin;
    
    // 压力控制
    QCheckBox* m_openPressCtrlCheck;
    QDoubleSpinBox* m_minPressSpin;
    QDoubleSpinBox* m_defaultPressSpin;
    QDoubleSpinBox* m_maxPressSpin;
    QDoubleSpinBox* m_pressIncreaseStepSpin;
    QSpinBox* m_pressIncreaseIntervalSpin;
    QSpinBox* m_pressCtrlTypeSpin;
    
    // 新增压力控制配置
    QLineEdit* m_ctrlPressPlcAddressEdit;           // 控制压力的写入地址
    QLineEdit* m_flowPlcAddressEdit;               // 当前流量数据地址
    QLineEdit* m_pressPeriodControlSettingEdit;     // 分时段控压设置
    QDoubleSpinBox* m_hstSpin;                       // 建筑高差
    QDoubleSpinBox* m_h0Spin;                        // 最不利点出流高差
    QDoubleSpinBox* m_rateSpin;                      // 水头损失
    QDoubleSpinBox* m_qsSpin;                        // 设计秒流量
    
    // 压测模式
    QCheckBox* m_stressTestCheck;
    QPushButton* m_cleanStressDataBtn;
    QLabel* m_stressDataCountLabel;
    QTimer* m_stressCountTimer;
};

#endif
