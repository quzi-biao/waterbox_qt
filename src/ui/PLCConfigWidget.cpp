#include "PLCConfigWidget.h"
#include "core/ConfigManager.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>

PLCConfigWidget::PLCConfigWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadConfig();
}

void PLCConfigWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* plcGroup = new QGroupBox("PLC 连接配置", this);
    QFormLayout* plcLayout = new QFormLayout(plcGroup);
    
    m_plcServerCheck = new QCheckBox(this);
    m_plcLocalHostEdit = new QLineEdit(this);
    m_plcPortSpin = new QSpinBox(this);
    m_plcPortSpin->setRange(1, 65535);
    m_plcPortSpin->setValue(102);
    
    m_plcProtocolCombo = new QComboBox(this);
    m_plcProtocolCombo->addItems({"S7", "Modbus"});
    
    m_plcSimulateCheck = new QCheckBox(this);
    
    plcLayout->addRow("启用 PLC 服务:", m_plcServerCheck);
    plcLayout->addRow("PLC 地址:", m_plcLocalHostEdit);
    plcLayout->addRow("PLC 端口:", m_plcPortSpin);
    plcLayout->addRow("协议类型:", m_plcProtocolCombo);
    plcLayout->addRow("模拟模式:", m_plcSimulateCheck);
    
    mainLayout->addWidget(plcGroup);
    
    QGroupBox* valveGroup = new QGroupBox("阀门控制配置", this);
    QFormLayout* valveLayout = new QFormLayout(valveGroup);
    
    m_plcCtrlCheck = new QCheckBox(this);
    m_plcValveReadAddressEdit = new QLineEdit(this);
    m_plcValveWriteAddressEdit = new QLineEdit(this);
    
    valveLayout->addRow("PLC 控制:", m_plcCtrlCheck);
    valveLayout->addRow("阀门读取地址:", m_plcValveReadAddressEdit);
    valveLayout->addRow("阀门写入地址:", m_plcValveWriteAddressEdit);
    
    mainLayout->addWidget(valveGroup);
    
    QPushButton* saveBtn = new QPushButton("保存配置", this);
    connect(saveBtn, &QPushButton::clicked, this, &PLCConfigWidget::saveConfig);
    
    mainLayout->addWidget(saveBtn);
    mainLayout->addStretch();
}

void PLCConfigWidget::loadConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    m_plcServerCheck->setChecked(config->get("plcServer", true).toBool());
    m_plcLocalHostEdit->setText(config->get("plcLocalHost", "192.168.1.11").toString());
    m_plcPortSpin->setValue(config->get("plcPort", 102).toInt());
    m_plcProtocolCombo->setCurrentText(config->get("plcProtocol", "S7").toString());
    m_plcSimulateCheck->setChecked(config->get("plcSimulate", false).toBool());
    m_plcCtrlCheck->setChecked(config->get("plcCtrl", false).toBool());
    m_plcValveReadAddressEdit->setText(config->get("plcValveReadAddress", "").toString());
    m_plcValveWriteAddressEdit->setText(config->get("plcValveWriteAddress", "").toString());
}

void PLCConfigWidget::saveConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    config->set("plcServer", m_plcServerCheck->isChecked());
    config->set("plcLocalHost", m_plcLocalHostEdit->text());
    config->set("plcPort", m_plcPortSpin->value());
    config->set("plcProtocol", m_plcProtocolCombo->currentText());
    config->set("plcSimulate", m_plcSimulateCheck->isChecked());
    config->set("plcCtrl", m_plcCtrlCheck->isChecked());
    config->set("plcValveReadAddress", m_plcValveReadAddressEdit->text());
    config->set("plcValveWriteAddress", m_plcValveWriteAddressEdit->text());
    
    config->save();
}
