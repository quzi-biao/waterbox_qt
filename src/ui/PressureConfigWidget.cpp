#include "PressureConfigWidget.h"
#include "core/ConfigManager.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>

PressureConfigWidget::PressureConfigWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadConfig();
}

void PressureConfigWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* basicGroup = new QGroupBox("基本压力控制配置", this);
    QFormLayout* basicLayout = new QFormLayout(basicGroup);
    
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
    
    basicLayout->addRow("启用压力控制:", m_openPressCtrlCheck);
    basicLayout->addRow("最小压力:", m_minPressSpin);
    basicLayout->addRow("默认压力:", m_defaultPressSpin);
    basicLayout->addRow("最大压力:", m_maxPressSpin);
    basicLayout->addRow("压力增长步长:", m_pressIncreaseStepSpin);
    basicLayout->addRow("压力增长间隔:", m_pressIncreaseIntervalSpin);
    basicLayout->addRow("控制类型:", m_pressCtrlTypeSpin);
    
    mainLayout->addWidget(basicGroup);
    
    QGroupBox* addressGroup = new QGroupBox("PLC 地址配置", this);
    QFormLayout* addressLayout = new QFormLayout(addressGroup);
    
    m_ctrlPressPlcAddressEdit = new QLineEdit(this);
    m_endPressAddressEdit = new QLineEdit(this);
    m_endPressDeviceCodeEdit = new QLineEdit(this);
    
    addressLayout->addRow("控制压力 PLC 地址:", m_ctrlPressPlcAddressEdit);
    addressLayout->addRow("末端压力地址:", m_endPressAddressEdit);
    addressLayout->addRow("末端压力设备代码:", m_endPressDeviceCodeEdit);
    
    mainLayout->addWidget(addressGroup);
    
    QGroupBox* endPressGroup = new QGroupBox("末端压力配置", this);
    QFormLayout* endPressLayout = new QFormLayout(endPressGroup);
    
    m_endPressReimCheck = new QCheckBox(this);
    
    m_endPressReimRateSpin = new QDoubleSpinBox(this);
    m_endPressReimRateSpin->setRange(0.1, 10.0);
    m_endPressReimRateSpin->setDecimals(2);
    
    m_endPressStandardSpin = new QDoubleSpinBox(this);
    m_endPressStandardSpin->setRange(0.0, 10.0);
    m_endPressStandardSpin->setSuffix(" MPa");
    m_endPressStandardSpin->setDecimals(2);
    
    m_endPressAvgTimeSpin = new QSpinBox(this);
    m_endPressAvgTimeSpin->setRange(1, 3600);
    m_endPressAvgTimeSpin->setSuffix(" s");
    
    endPressLayout->addRow("末端压力补偿:", m_endPressReimCheck);
    endPressLayout->addRow("补偿比率:", m_endPressReimRateSpin);
    endPressLayout->addRow("标准压力:", m_endPressStandardSpin);
    endPressLayout->addRow("平均时间:", m_endPressAvgTimeSpin);
    
    mainLayout->addWidget(endPressGroup);
    
    QPushButton* saveBtn = new QPushButton("保存配置", this);
    connect(saveBtn, &QPushButton::clicked, this, &PressureConfigWidget::saveConfig);
    
    mainLayout->addWidget(saveBtn);
    mainLayout->addStretch();
}

void PressureConfigWidget::loadConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    m_openPressCtrlCheck->setChecked(config->get("openPressCtrl", false).toBool());
    m_minPressSpin->setValue(config->get("minPress", 0.2).toDouble());
    m_defaultPressSpin->setValue(config->get("defaultPress", 0.3).toDouble());
    m_maxPressSpin->setValue(config->get("maxPress", 0.5).toDouble());
    m_pressIncreaseStepSpin->setValue(config->get("pressIncreaseStep", 0.01).toDouble());
    m_pressIncreaseIntervalSpin->setValue(config->get("pressIncreaseInterval", 300).toInt());
    m_ctrlPressPlcAddressEdit->setText(config->get("ctrlPressPlcAddress", "").toString());
    m_endPressAddressEdit->setText(config->get("endPressAddress", "").toString());
    m_endPressDeviceCodeEdit->setText(config->get("endPressDeviceCode", "").toString());
    m_endPressReimCheck->setChecked(config->get("endPressReim", false).toBool());
    m_endPressReimRateSpin->setValue(config->get("endPressReimRate", 1.0).toDouble());
    m_endPressStandardSpin->setValue(config->get("endPressStandard", 0.3).toDouble());
    m_endPressAvgTimeSpin->setValue(config->get("endPressAvgTime", 60).toInt());
    m_pressCtrlTypeSpin->setValue(config->get("pressCtrlType", 0).toInt());
}

void PressureConfigWidget::saveConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    config->set("openPressCtrl", m_openPressCtrlCheck->isChecked());
    config->set("minPress", m_minPressSpin->value());
    config->set("defaultPress", m_defaultPressSpin->value());
    config->set("maxPress", m_maxPressSpin->value());
    config->set("pressIncreaseStep", m_pressIncreaseStepSpin->value());
    config->set("pressIncreaseInterval", m_pressIncreaseIntervalSpin->value());
    config->set("ctrlPressPlcAddress", m_ctrlPressPlcAddressEdit->text());
    config->set("endPressAddress", m_endPressAddressEdit->text());
    config->set("endPressDeviceCode", m_endPressDeviceCodeEdit->text());
    config->set("endPressReim", m_endPressReimCheck->isChecked());
    config->set("endPressReimRate", m_endPressReimRateSpin->value());
    config->set("endPressStandard", m_endPressStandardSpin->value());
    config->set("endPressAvgTime", m_endPressAvgTimeSpin->value());
    config->set("pressCtrlType", m_pressCtrlTypeSpin->value());
    
    config->save();
}
