#include "ConfigDialog.h"
#include "core/ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>

ConfigDialog::ConfigDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("系统配置");
    resize(600, 700);
    
    setupUI();
    loadConfig();
}

void ConfigDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* plcGroup = new QGroupBox("PLC 配置", this);
    QFormLayout* plcLayout = new QFormLayout(plcGroup);
    
    m_plcHostEdit = new QLineEdit(this);
    m_plcPortSpin = new QSpinBox(this);
    m_plcPortSpin->setRange(1, 65535);
    m_plcPortSpin->setValue(102);
    
    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItems({"S7", "Modbus"});
    
    plcLayout->addRow("PLC 地址:", m_plcHostEdit);
    plcLayout->addRow("PLC 端口:", m_plcPortSpin);
    plcLayout->addRow("协议:", m_protocolCombo);
    
    mainLayout->addWidget(plcGroup);
    
    QGroupBox* reportGroup = new QGroupBox("数据上报配置", this);
    QFormLayout* reportLayout = new QFormLayout(reportGroup);
    
    m_reportUrlEdit = new QLineEdit(this);
    m_reportProtocolCombo = new QComboBox(this);
    m_reportProtocolCombo->addItems({"HTTP", "HTTPS"});
    
    reportLayout->addRow("上报地址:", m_reportUrlEdit);
    reportLayout->addRow("上报协议:", m_reportProtocolCombo);
    
    mainLayout->addWidget(reportGroup);
    
    QGroupBox* intervalGroup = new QGroupBox("采集间隔配置", this);
    QFormLayout* intervalLayout = new QFormLayout(intervalGroup);
    
    m_collectIntervalSpin = new QSpinBox(this);
    m_collectIntervalSpin->setRange(500, 3600000);
    m_collectIntervalSpin->setSuffix(" ms");
    m_collectIntervalSpin->setValue(10000);
    
    m_reportIntervalSpin = new QSpinBox(this);
    m_reportIntervalSpin->setRange(500, 3600000);
    m_reportIntervalSpin->setSuffix(" ms");
    m_reportIntervalSpin->setValue(10000);
    
    intervalLayout->addRow("采集间隔:", m_collectIntervalSpin);
    intervalLayout->addRow("上报间隔:", m_reportIntervalSpin);
    
    mainLayout->addWidget(intervalGroup);
    
    QGroupBox* controlGroup = new QGroupBox("控制器配置", this);
    QFormLayout* controlLayout = new QFormLayout(controlGroup);
    
    m_ctrlPressAddrEdit = new QLineEdit(this);
    m_endPressAddrEdit = new QLineEdit(this);
    m_flowAddrEdit = new QLineEdit(this);
    
    controlLayout->addRow("控制压力地址:", m_ctrlPressAddrEdit);
    controlLayout->addRow("末端压力地址:", m_endPressAddrEdit);
    controlLayout->addRow("流量地址:", m_flowAddrEdit);
    
    mainLayout->addWidget(controlGroup);
    
    QGroupBox* addressGroup = new QGroupBox("PLC 地址配置", this);
    QVBoxLayout* addressLayout = new QVBoxLayout(addressGroup);
    
    m_addressTable = new QTableWidget(this);
    m_addressTable->setColumnCount(3);
    m_addressTable->setHorizontalHeaderLabels({"地址", "名称", "可写"});
    m_addressTable->horizontalHeader()->setStretchLastSection(true);
    
    addressLayout->addWidget(m_addressTable);
    
    QHBoxLayout* addressBtnLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("添加", this);
    QPushButton* removeBtn = new QPushButton("删除", this);
    connect(addBtn, &QPushButton::clicked, this, &ConfigDialog::onAddAddress);
    connect(removeBtn, &QPushButton::clicked, this, &ConfigDialog::onRemoveAddress);
    
    addressBtnLayout->addWidget(addBtn);
    addressBtnLayout->addWidget(removeBtn);
    addressBtnLayout->addStretch();
    
    addressLayout->addLayout(addressBtnLayout);
    
    mainLayout->addWidget(addressGroup);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定", this);
    QPushButton* cancelBtn = new QPushButton("取消", this);
    
    connect(okBtn, &QPushButton::clicked, this, &ConfigDialog::onAccept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(btnLayout);
}

void ConfigDialog::loadConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    m_plcHostEdit->setText(config->plcHost());
    m_plcPortSpin->setValue(config->plcPort());
    m_protocolCombo->setCurrentText(config->plcProtocol());
    
    m_reportUrlEdit->setText(config->reportUrl());
    m_reportProtocolCombo->setCurrentText(config->reportProtocol());
    
    m_collectIntervalSpin->setValue(config->collectInterval());
    m_reportIntervalSpin->setValue(config->reportInterval());
    
    m_ctrlPressAddrEdit->setText(config->get("ctrl_pressure_addr", "V100").toString());
    m_endPressAddrEdit->setText(config->get("end_pressure_addr", "V200").toString());
    m_flowAddrEdit->setText(config->get("flow_addr", "V300").toString());
}

void ConfigDialog::saveConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    config->set("plc_host", m_plcHostEdit->text());
    config->set("plc_port", m_plcPortSpin->value());
    config->set("plc_protocol", m_protocolCombo->currentText());
    
    config->set("report_url", m_reportUrlEdit->text());
    config->set("report_protocol", m_reportProtocolCombo->currentText());
    
    config->set("collect_interval", m_collectIntervalSpin->value());
    config->set("report_interval", m_reportIntervalSpin->value());
    
    config->set("ctrl_pressure_addr", m_ctrlPressAddrEdit->text());
    config->set("end_pressure_addr", m_endPressAddrEdit->text());
    config->set("flow_addr", m_flowAddrEdit->text());
    
    for (int row = 0; row < m_addressTable->rowCount(); ++row) {
        QString address = m_addressTable->item(row, 0)->text();
        QString name = m_addressTable->item(row, 1)->text();
        bool writable = m_addressTable->item(row, 2)->checkState() == Qt::Checked;
        
        config->set(QString("addr_name_%1").arg(address), name);
        config->set(QString("addr_writable_%1").arg(address), writable);
    }
}

void ConfigDialog::onAccept() {
    saveConfig();
    accept();
}

void ConfigDialog::onAddAddress() {
    int row = m_addressTable->rowCount();
    m_addressTable->insertRow(row);
    
    m_addressTable->setItem(row, 0, new QTableWidgetItem("V0"));
    m_addressTable->setItem(row, 1, new QTableWidgetItem("新地址"));
    
    QTableWidgetItem* checkItem = new QTableWidgetItem();
    checkItem->setCheckState(Qt::Unchecked);
    m_addressTable->setItem(row, 2, checkItem);
}

void ConfigDialog::onRemoveAddress() {
    int currentRow = m_addressTable->currentRow();
    if (currentRow >= 0) {
        m_addressTable->removeRow(currentRow);
    }
}
