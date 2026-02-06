#include "TankConfigWidget.h"
#include "core/ConfigManager.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>

TankConfigWidget::TankConfigWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadConfig();
}

void TankConfigWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
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
    
    mainLayout->addWidget(tankGroup);
    
    QPushButton* saveBtn = new QPushButton("保存配置", this);
    connect(saveBtn, &QPushButton::clicked, this, &TankConfigWidget::saveConfig);
    
    mainLayout->addWidget(saveBtn);
    mainLayout->addStretch();
}

void TankConfigWidget::loadConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    m_boxLongSpin->setValue(config->get("boxLong", 10.0).toDouble());
    m_boxWideSpin->setValue(config->get("boxWide", 5.0).toDouble());
    m_boxHighSpin->setValue(config->get("boxHigh", 4.0).toDouble());
    m_boxNumSpin->setValue(config->get("boxNum", 1).toInt());
}

void TankConfigWidget::saveConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    config->set("boxLong", m_boxLongSpin->value());
    config->set("boxWide", m_boxWideSpin->value());
    config->set("boxHigh", m_boxHighSpin->value());
    config->set("boxNum", m_boxNumSpin->value());
    
    config->save();
}
