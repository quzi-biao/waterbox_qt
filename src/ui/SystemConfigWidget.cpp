#include "SystemConfigWidget.h"
#include "core/ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QLabel>

SystemConfigWidget::SystemConfigWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadConfig();
}

void SystemConfigWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 创建滚动区域
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    QWidget* scrollWidget = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    
    // 云端配置 - 单独一行，输入框占满宽度
    QGroupBox* cloudGroup = new QGroupBox("云端配置", this);
    QVBoxLayout* cloudLayout = new QVBoxLayout(cloudGroup);
    
    QHBoxLayout* serviceLayout = new QHBoxLayout();
    QLabel* serviceLabel = new QLabel("服务地址:", this);
    serviceLabel->setMinimumWidth(100);
    m_serviceAddressEdit = new QLineEdit(this);
    m_serviceAddressEdit->setMinimumWidth(400);
    serviceLayout->addWidget(serviceLabel);
    serviceLayout->addWidget(m_serviceAddressEdit, 1);
    cloudLayout->addLayout(serviceLayout);
    
    scrollLayout->addWidget(cloudGroup);
    
    // 左右分栏布局
    QHBoxLayout* columnsLayout = new QHBoxLayout();
    
    // 左列
    QVBoxLayout* leftColumn = new QVBoxLayout();
    
    // PLC 配置
    QGroupBox* plcGroup = new QGroupBox("PLC 连接配置", this);
    QFormLayout* plcLayout = new QFormLayout(plcGroup);
    
    m_plcServerCheck = new QCheckBox(this);
    m_plcLocalHostEdit = new QLineEdit(this);
    m_plcPortSpin = new QSpinBox(this);
    m_plcPortSpin->setRange(1, 65535);
    m_plcPortSpin->setValue(102);
    
    m_plcProtocolCombo = new QComboBox(this);
    m_plcProtocolCombo->addItem("Modbus");  // 先尝试 Modbus
    m_plcProtocolCombo->addItem("S7");
    
    plcLayout->addRow("启用 PLC 服务:", m_plcServerCheck);
    plcLayout->addRow("PLC 地址:", m_plcLocalHostEdit);
    plcLayout->addRow("PLC 端口:", m_plcPortSpin);
    plcLayout->addRow("协议类型:", m_plcProtocolCombo);
    
    leftColumn->addWidget(plcGroup);
    
    // 水箱参数
    QGroupBox* tankGroup = new QGroupBox("水箱尺寸配置", this);
    QFormLayout* tankLayout = new QFormLayout(tankGroup);
    
    m_boxLongSpin = new QDoubleSpinBox(this);
    m_boxLongSpin->setRange(0.1, 1000.0);
    m_boxLongSpin->setSuffix(" m");
    m_boxLongSpin->setDecimals(2);
    
    m_boxWideSpin = new QDoubleSpinBox(this);
    m_boxWideSpin->setRange(0.1, 1000.0);
    m_boxWideSpin->setSuffix(" m");
    m_boxWideSpin->setDecimals(2);
    
    m_boxHighSpin = new QDoubleSpinBox(this);
    m_boxHighSpin->setRange(0.1, 100.0);
    m_boxHighSpin->setSuffix(" m");
    m_boxHighSpin->setDecimals(2);
    
    m_boxNumSpin = new QSpinBox(this);
    m_boxNumSpin->setRange(1, 100);
    
    tankLayout->addRow("水箱长度:", m_boxLongSpin);
    tankLayout->addRow("水箱宽度:", m_boxWideSpin);
    tankLayout->addRow("水箱高度:", m_boxHighSpin);
    tankLayout->addRow("水箱数量:", m_boxNumSpin);
    
    leftColumn->addWidget(tankGroup);
    leftColumn->addStretch();
    
    // 右列
    QVBoxLayout* rightColumn = new QVBoxLayout();
    
    // 压力控制
    QGroupBox* pressGroup = new QGroupBox("压力控制配置", this);
    QFormLayout* pressLayout = new QFormLayout(pressGroup);
    
    m_openPressCtrlCheck = new QCheckBox(this);
    
    m_minPressSpin = new QDoubleSpinBox(this);
    m_minPressSpin->setRange(0.0, 10.0);
    m_minPressSpin->setSuffix(" MPa");
    m_minPressSpin->setDecimals(2);
    
    m_defaultPressSpin = new QDoubleSpinBox(this);
    m_defaultPressSpin->setRange(0.0, 10.0);
    m_defaultPressSpin->setSuffix(" MPa");
    m_defaultPressSpin->setDecimals(2);
    
    m_maxPressSpin = new QDoubleSpinBox(this);
    m_maxPressSpin->setRange(0.0, 10.0);
    m_maxPressSpin->setSuffix(" MPa");
    m_maxPressSpin->setDecimals(2);
    
    m_pressIncreaseStepSpin = new QDoubleSpinBox(this);
    m_pressIncreaseStepSpin->setRange(0.001, 1.0);
    m_pressIncreaseStepSpin->setSuffix(" MPa");
    m_pressIncreaseStepSpin->setDecimals(3);
    
    m_pressIncreaseIntervalSpin = new QSpinBox(this);
    m_pressIncreaseIntervalSpin->setRange(1, 3600);
    m_pressIncreaseIntervalSpin->setSuffix(" s");
    
    m_pressCtrlTypeSpin = new QSpinBox(this);
    m_pressCtrlTypeSpin->setRange(0, 10);
    
    // 新增字段
    m_ctrlPressPlcAddressEdit = new QLineEdit(this);
    m_pressPeriodControlSettingEdit = new QLineEdit(this);
    m_pressCalculatorEdit = new QLineEdit(this);
    m_endPressDeviceCodeEdit = new QLineEdit(this);
    
    m_endPressReimCheck = new QCheckBox(this);
    
    m_endPressReimRateSpin = new QDoubleSpinBox(this);
    m_endPressReimRateSpin->setRange(0.0, 10.0);
    m_endPressReimRateSpin->setDecimals(2);
    
    m_endPressStandardSpin = new QDoubleSpinBox(this);
    m_endPressStandardSpin->setRange(0.0, 10.0);
    m_endPressStandardSpin->setSuffix(" MPa");
    m_endPressStandardSpin->setDecimals(2);
    
    m_endPressAvgTimeSpin = new QSpinBox(this);
    m_endPressAvgTimeSpin->setRange(1, 3600);
    m_endPressAvgTimeSpin->setSuffix(" s");
    
    pressLayout->addRow("启用压力控制:", m_openPressCtrlCheck);
    pressLayout->addRow("控制类型:", m_pressCtrlTypeSpin);
    pressLayout->addRow("最小压力:", m_minPressSpin);
    pressLayout->addRow("默认压力:", m_defaultPressSpin);
    pressLayout->addRow("最大压力:", m_maxPressSpin);
    pressLayout->addRow("压力增长步长:", m_pressIncreaseStepSpin);
    pressLayout->addRow("压力增长间隔:", m_pressIncreaseIntervalSpin);
    pressLayout->addRow("控制压力写入地址:", m_ctrlPressPlcAddressEdit);
    pressLayout->addRow("分时段控压设置:", m_pressPeriodControlSettingEdit);
    pressLayout->addRow("压力计算公式:", m_pressCalculatorEdit);
    pressLayout->addRow("末端压力设备编码:", m_endPressDeviceCodeEdit);
    pressLayout->addRow("末端压力补偿:", m_endPressReimCheck);
    pressLayout->addRow("末端压力补偿比率:", m_endPressReimRateSpin);
    pressLayout->addRow("末端压力标准值:", m_endPressStandardSpin);
    pressLayout->addRow("末端压力均值时间:", m_endPressAvgTimeSpin);
    
    rightColumn->addWidget(pressGroup);
    rightColumn->addStretch();
    
    // 添加左右列到列布局
    columnsLayout->addLayout(leftColumn, 1);
    columnsLayout->addLayout(rightColumn, 1);
    
    scrollLayout->addLayout(columnsLayout);
    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);
    
    mainLayout->addWidget(scrollArea);
    
    // 保存按钮
    QPushButton* saveBtn = new QPushButton("保存所有配置", this);
    saveBtn->setMinimumHeight(40);
    connect(saveBtn, &QPushButton::clicked, this, &SystemConfigWidget::saveConfig);
    
    mainLayout->addWidget(saveBtn);
}

void SystemConfigWidget::loadConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    // 云端配置
    m_serviceAddressEdit->setText(config->get("serviceAddress", "http://up.waters-ai.work").toString());
    
    // PLC 配置
    m_plcServerCheck->setChecked(config->get("plcServer", true).toBool());
    m_plcLocalHostEdit->setText(config->get("plcLocalHost", "192.168.1.11").toString());
    m_plcPortSpin->setValue(config->get("plcPort", 102).toInt());
    m_plcProtocolCombo->setCurrentText(config->get("plcProtocol", "S7").toString());
    
    // 水箱参数
    m_boxLongSpin->setValue(config->get("boxLong", 10.0).toDouble());
    m_boxWideSpin->setValue(config->get("boxWide", 5.0).toDouble());
    m_boxHighSpin->setValue(config->get("boxHigh", 4.0).toDouble());
    m_boxNumSpin->setValue(config->get("boxNum", 1).toInt());
    
    // 压力控制
    m_openPressCtrlCheck->setChecked(config->get("openPressCtrl", false).toBool());
    m_minPressSpin->setValue(config->get("minPress", 0.2).toDouble());
    m_defaultPressSpin->setValue(config->get("defaultPress", 0.3).toDouble());
    m_maxPressSpin->setValue(config->get("maxPress", 0.5).toDouble());
    m_pressIncreaseStepSpin->setValue(config->get("pressIncreaseStep", 0.01).toDouble());
    m_pressIncreaseIntervalSpin->setValue(config->get("pressIncreaseInterval", 300).toInt());
    m_pressCtrlTypeSpin->setValue(config->get("pressCtrlType", 0).toInt());
    
    // 新增字段
    m_ctrlPressPlcAddressEdit->setText(config->get("ctrlPressPlcAddress", "").toString());
    m_pressPeriodControlSettingEdit->setText(config->get("pressPeriodControlSetting", "").toString());
    m_pressCalculatorEdit->setText(config->get("pressCalculator", "").toString());
    m_endPressDeviceCodeEdit->setText(config->get("endPressDeviceCode", "").toString());
    m_endPressReimCheck->setChecked(config->get("endPressReim", false).toBool());
    m_endPressReimRateSpin->setValue(config->get("endPressReimRate", 1.0).toDouble());
    m_endPressStandardSpin->setValue(config->get("endPressStandard", 0.3).toDouble());
    m_endPressAvgTimeSpin->setValue(config->get("endPressAvgTime", 60).toInt());
}

void SystemConfigWidget::saveConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    // 云端配置
    config->set("serviceAddress", m_serviceAddressEdit->text());
    
    // PLC 配置
    config->set("plcServer", m_plcServerCheck->isChecked());
    config->set("plcLocalHost", m_plcLocalHostEdit->text());
    config->set("plcPort", m_plcPortSpin->value());
    config->set("plcProtocol", m_plcProtocolCombo->currentText());
    
    // 水箱参数
    config->set("boxLong", m_boxLongSpin->value());
    config->set("boxWide", m_boxWideSpin->value());
    config->set("boxHigh", m_boxHighSpin->value());
    config->set("boxNum", m_boxNumSpin->value());
    
    // 压力控制
    config->set("openPressCtrl", m_openPressCtrlCheck->isChecked());
    config->set("minPress", m_minPressSpin->value());
    config->set("defaultPress", m_defaultPressSpin->value());
    config->set("maxPress", m_maxPressSpin->value());
    config->set("pressIncreaseStep", m_pressIncreaseStepSpin->value());
    config->set("pressIncreaseInterval", m_pressIncreaseIntervalSpin->value());
    config->set("pressCtrlType", m_pressCtrlTypeSpin->value());
    
    // 新增字段
    config->set("ctrlPressPlcAddress", m_ctrlPressPlcAddressEdit->text());
    config->set("pressPeriodControlSetting", m_pressPeriodControlSettingEdit->text());
    config->set("pressCalculator", m_pressCalculatorEdit->text());
    config->set("endPressDeviceCode", m_endPressDeviceCodeEdit->text());
    config->set("endPressReim", m_endPressReimCheck->isChecked());
    config->set("endPressReimRate", m_endPressReimRateSpin->value());
    config->set("endPressStandard", m_endPressStandardSpin->value());
    config->set("endPressAvgTime", m_endPressAvgTimeSpin->value());
    
    config->save();
    
    emit configSaved();
}
