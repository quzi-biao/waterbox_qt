#include "SystemConfigWidget.h"
#include "core/ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>

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
    
    m_dataCollectIntervalSpin = new QSpinBox(this);
    m_dataCollectIntervalSpin->setRange(1000, 600000);
    m_dataCollectIntervalSpin->setSingleStep(1000);
    m_dataCollectIntervalSpin->setSuffix(" ms");
    
    plcLayout->addRow("启用 PLC 服务:", m_plcServerCheck);
    plcLayout->addRow("PLC 地址:", m_plcLocalHostEdit);
    plcLayout->addRow("PLC 端口:", m_plcPortSpin);
    plcLayout->addRow("协议类型:", m_plcProtocolCombo);
    plcLayout->addRow("数据采集间隔:", m_dataCollectIntervalSpin);
    
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
    m_flowPlcAddressEdit = new QLineEdit(this);
    m_pressPeriodControlSettingEdit = new QLineEdit(this);
    
    m_hstSpin = new QDoubleSpinBox(this);
    m_hstSpin->setRange(0.0, 1000.0);
    m_hstSpin->setDecimals(2);
    m_hstSpin->setSuffix(" m");
    
    m_h0Spin = new QDoubleSpinBox(this);
    m_h0Spin->setRange(0.0, 1000.0);
    m_h0Spin->setDecimals(2);
    m_h0Spin->setSuffix(" m");
    
    m_rateSpin = new QDoubleSpinBox(this);
    m_rateSpin->setRange(0.0, 1.0);
    m_rateSpin->setDecimals(3);
    
    m_qsSpin = new QDoubleSpinBox(this);
    m_qsSpin->setRange(0.0, 10000.0);
    m_qsSpin->setDecimals(2);
    m_qsSpin->setSuffix(" m³/h");
    
    pressLayout->addRow("启用压力控制:", m_openPressCtrlCheck);
    pressLayout->addRow("控制类型:", m_pressCtrlTypeSpin);
    pressLayout->addRow("最小压力:", m_minPressSpin);
    pressLayout->addRow("默认压力:", m_defaultPressSpin);
    pressLayout->addRow("最大压力:", m_maxPressSpin);
    pressLayout->addRow("压力增长步长:", m_pressIncreaseStepSpin);
    pressLayout->addRow("压力增长间隔:", m_pressIncreaseIntervalSpin);
    pressLayout->addRow("控制压力写入地址:", m_ctrlPressPlcAddressEdit);
    pressLayout->addRow("当前流量数据地址:", m_flowPlcAddressEdit);
    pressLayout->addRow("分时段控压设置:", m_pressPeriodControlSettingEdit);
    pressLayout->addRow("建筑高差:", m_hstSpin);
    pressLayout->addRow("最不利点出流高差:", m_h0Spin);
    pressLayout->addRow("水头损失:", m_rateSpin);
    pressLayout->addRow("设计秒流量:", m_qsSpin);
    
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
    m_dataCollectIntervalSpin->setValue(config->get("pumpMetricReadInterval", 10000).toInt());
    
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
    m_flowPlcAddressEdit->setText(config->get("flowPlcAddress", "").toString());
    m_pressPeriodControlSettingEdit->setText(config->get("pressPeriodControlSetting", "").toString());
    // 压力计算公式（从 JSON 解析）
    QString pressCalcStr = config->get("pressCalculator", "").toString();
    if (!pressCalcStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(pressCalcStr.toUtf8());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            m_hstSpin->setValue(obj.value("hst").toString("0").toDouble());
            m_h0Spin->setValue(obj.value("h0").toString("0").toDouble());
            m_rateSpin->setValue(obj.value("rate").toString("0").toDouble());
            m_qsSpin->setValue(obj.value("qs").toString("0").toDouble());
        }
    }
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
    config->set("pumpMetricReadInterval", m_dataCollectIntervalSpin->value());
    
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
    config->set("flowPlcAddress", m_flowPlcAddressEdit->text());
    config->set("pressPeriodControlSetting", m_pressPeriodControlSettingEdit->text());
    // 压力计算公式（合成 JSON）
    QJsonObject pressCalcObj;
    pressCalcObj["realSetting"] = QJsonObject();
    pressCalcObj["hst"] = QString::number(m_hstSpin->value());
    pressCalcObj["h0"] = QString::number(m_h0Spin->value());
    pressCalcObj["rate"] = QString::number(m_rateSpin->value());
    pressCalcObj["qs"] = QString::number(m_qsSpin->value());
    config->set("pressCalculator", QString::fromUtf8(QJsonDocument(pressCalcObj).toJson(QJsonDocument::Compact)));
    
    config->save();
    
    emit configSaved();
}
