#ifndef SYSTEMCONFIGWIDGET_H
#define SYSTEMCONFIGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>

class SystemConfigWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit SystemConfigWidget(QWidget* parent = nullptr);
    
    void loadConfig();
    void saveConfig();
    
signals:
    void configSaved();
    
private:
    void setupUI();
    
    // 云端配置
    QLineEdit* m_serviceAddressEdit;
    
    // PLC 配置
    QCheckBox* m_plcServerCheck;
    QLineEdit* m_plcLocalHostEdit;
    QSpinBox* m_plcPortSpin;
    QComboBox* m_plcProtocolCombo;
    
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
    QLineEdit* m_pressPeriodControlSettingEdit;     // 分时段控压设置
    QLineEdit* m_pressCalculatorEdit;               // 压力计算公式
    QLineEdit* m_endPressDeviceCodeEdit;            // 末端压力设备编码
    QCheckBox* m_endPressReimCheck;                 // 是否开启末端压力补偿
    QDoubleSpinBox* m_endPressReimRateSpin;         // 末端压力补偿比率
    QDoubleSpinBox* m_endPressStandardSpin;         // 末端压力标准值
    QSpinBox* m_endPressAvgTimeSpin;                // 末端压力均值时间
};

#endif
