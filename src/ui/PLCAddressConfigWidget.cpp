#include "PLCAddressConfigWidget.h"
#include "database/DatabaseManager.h"
#include "network/HttpSender.h"
#include "entity/KeyValue.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>

PLCAddressConfigWidget::PLCAddressConfigWidget(QWidget* parent) 
    : QWidget(parent), m_httpSender(nullptr) {
    setupUI();
    loadConfig();
}

void PLCAddressConfigWidget::setHttpSender(HttpSender* sender) {
    m_httpSender = sender;
}

void PLCAddressConfigWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 创建表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"ID", "PLC地址", "数据类型", "单位", "可写"});
    
    // 设置列宽
    m_table->setColumnWidth(0, 80);
    m_table->setColumnWidth(1, 200);
    m_table->setColumnWidth(2, 150);
    m_table->setColumnWidth(3, 100);
    m_table->setColumnWidth(4, 80);
    
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    
    mainLayout->addWidget(m_table);
    
    // 按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    m_addBtn = new QPushButton("添加地址", this);
    m_deleteBtn = new QPushButton("删除地址", this);
    m_saveBtn = new QPushButton("保存并上报", this);
    m_saveBtn->setMinimumHeight(40);
    
    connect(m_addBtn, &QPushButton::clicked, this, &PLCAddressConfigWidget::onAddRow);
    connect(m_deleteBtn, &QPushButton::clicked, this, &PLCAddressConfigWidget::onDeleteRow);
    connect(m_saveBtn, &QPushButton::clicked, this, &PLCAddressConfigWidget::onSaveConfig);
    
    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_deleteBtn);
    btnLayout->addStretch();
    
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(m_saveBtn);
}

void PLCAddressConfigWidget::loadConfig() {
    loadIndicators();
    
    m_table->setRowCount(m_indicators.size());
    
    for (int i = 0; i < m_indicators.size(); ++i) {
        const MetricIndicator& indicator = m_indicators[i];
        
        // ID
        QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(indicator.id()));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 0, idItem);
        
        // PLC地址
        m_table->setItem(i, 1, new QTableWidgetItem(indicator.address()));
        
        // 数据类型
        QComboBox* typeCombo = new QComboBox();
        typeCombo->addItem("BOOL", MetricIndicatorDataType::BOOL);
        typeCombo->addItem("BYTE", MetricIndicatorDataType::BYTE);
        typeCombo->addItem("INT16", MetricIndicatorDataType::INT16);
        typeCombo->addItem("UINT16", MetricIndicatorDataType::UINT16);
        typeCombo->addItem("INT32", MetricIndicatorDataType::INT32);
        typeCombo->addItem("UINT32", MetricIndicatorDataType::UINT32);
        typeCombo->addItem("FLOAT32", MetricIndicatorDataType::FLOAT32);
        typeCombo->addItem("FLOAT64", MetricIndicatorDataType::FLOAT64);
        typeCombo->setCurrentIndex(typeCombo->findData(indicator.dataType()));
        m_table->setCellWidget(i, 2, typeCombo);
        
        // 单位
        m_table->setItem(i, 3, new QTableWidgetItem(indicator.unit()));
        
        // 可写
        QWidget* checkWidget = new QWidget();
        QHBoxLayout* checkLayout = new QHBoxLayout(checkWidget);
        QCheckBox* writableCheck = new QCheckBox();
        writableCheck->setChecked(indicator.writable());
        checkLayout->addWidget(writableCheck);
        checkLayout->setAlignment(Qt::AlignCenter);
        checkLayout->setContentsMargins(0, 0, 0, 0);
        m_table->setCellWidget(i, 4, checkWidget);
    }
}

void PLCAddressConfigWidget::saveConfig() {
    saveIndicators();
}

void PLCAddressConfigWidget::reloadConfig() {
    qInfo() << "重新加载 PLC 地址配置";
    loadConfig();
}

void PLCAddressConfigWidget::onAddRow() {
    int row = m_table->rowCount();
    m_table->insertRow(row);
    
    // 生成新ID
    qint64 newId = QDateTime::currentMSecsSinceEpoch();
    
    // ID
    QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(newId));
    idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
    m_table->setItem(row, 0, idItem);
    
    // PLC地址
    m_table->setItem(row, 1, new QTableWidgetItem("DB1.DBD0"));
    
    // 数据类型
    QComboBox* typeCombo = new QComboBox();
    typeCombo->addItem("BOOL", MetricIndicatorDataType::BOOL);
    typeCombo->addItem("BYTE", MetricIndicatorDataType::BYTE);
    typeCombo->addItem("INT16", MetricIndicatorDataType::INT16);
    typeCombo->addItem("UINT16", MetricIndicatorDataType::UINT16);
    typeCombo->addItem("INT32", MetricIndicatorDataType::INT32);
    typeCombo->addItem("UINT32", MetricIndicatorDataType::UINT32);
    typeCombo->addItem("FLOAT32", MetricIndicatorDataType::FLOAT32);
    typeCombo->addItem("FLOAT64", MetricIndicatorDataType::FLOAT64);
    typeCombo->setCurrentIndex(2); // 默认 INT16
    m_table->setCellWidget(row, 2, typeCombo);
    
    // 单位
    m_table->setItem(row, 3, new QTableWidgetItem(""));
    
    // 可写
    QWidget* checkWidget = new QWidget();
    QHBoxLayout* checkLayout = new QHBoxLayout(checkWidget);
    QCheckBox* writableCheck = new QCheckBox();
    writableCheck->setChecked(false);
    checkLayout->addWidget(writableCheck);
    checkLayout->setAlignment(Qt::AlignCenter);
    checkLayout->setContentsMargins(0, 0, 0, 0);
    m_table->setCellWidget(row, 4, checkWidget);
}

void PLCAddressConfigWidget::onDeleteRow() {
    int currentRow = m_table->currentRow();
    if (currentRow >= 0) {
        m_table->removeRow(currentRow);
    } else {
        QMessageBox::warning(this, "提示", "请先选择要删除的行");
    }
}

void PLCAddressConfigWidget::onSaveConfig() {
    saveIndicators();
    uploadToServer();
    QMessageBox::information(this, "成功", "配置已保存并上报到服务端");
}

void PLCAddressConfigWidget::loadIndicators() {
    m_indicators.clear();
    
    DatabaseManager* db = DatabaseManager::instance();
    QVariant indicatorValue = db->getKeyValue("METRIC_INDICATOR_KEY");
    
    if (!indicatorValue.isNull() && !indicatorValue.toString().isEmpty()) {
        QString jsonStr = indicatorValue.toString();
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        
        if (doc.isArray()) {
            QJsonArray array = doc.array();
            for (const QJsonValue& val : array) {
                if (val.isObject()) {
                    MetricIndicator indicator = MetricIndicator::fromJson(val.toObject());
                    m_indicators.append(indicator);
                }
            }
        }
    }
}

void PLCAddressConfigWidget::saveIndicators() {
    m_indicators.clear();
    
    for (int i = 0; i < m_table->rowCount(); ++i) {
        MetricIndicator indicator;
        
        // ID
        indicator.setId(m_table->item(i, 0)->text().toLongLong());
        
        // PLC地址
        indicator.setAddress(m_table->item(i, 1)->text());
        
        // 数据类型
        QComboBox* typeCombo = qobject_cast<QComboBox*>(m_table->cellWidget(i, 2));
        if (typeCombo) {
            indicator.setDataType(typeCombo->currentData().toInt());
        }
        
        // 单位
        indicator.setUnit(m_table->item(i, 3)->text());
        
        // 可写
        QWidget* checkWidget = m_table->cellWidget(i, 4);
        if (checkWidget) {
            QCheckBox* writableCheck = checkWidget->findChild<QCheckBox*>();
            if (writableCheck) {
                indicator.setWritable(writableCheck->isChecked());
            }
        }
        
        m_indicators.append(indicator);
    }
    
    // 保存到数据库
    QJsonArray array;
    for (const MetricIndicator& indicator : m_indicators) {
        array.append(indicator.toJson());
    }
    
    QString jsonStr = QJsonDocument(array).toJson(QJsonDocument::Compact);
    DatabaseManager* db = DatabaseManager::instance();
    db->saveKeyValue("METRIC_INDICATOR_KEY", jsonStr);
}

void PLCAddressConfigWidget::uploadToServer() {
    if (!m_httpSender || !m_httpSender->isInitialized()) {
        qWarning() << "HttpSender 未初始化，无法上报配置";
        return;
    }
    
    // 构建 KeyValue 数组
    QJsonArray kvArray;
    
    QJsonObject kv;
    kv["itemKey"] = "METRIC_INDICATOR_KEY";
    
    // 将 indicators 转换为 JSON 字符串
    QJsonArray indicatorArray;
    for (const MetricIndicator& indicator : m_indicators) {
        indicatorArray.append(indicator.toJson());
    }
    kv["itemValue"] = QString::fromUtf8(QJsonDocument(indicatorArray).toJson(QJsonDocument::Compact));
    kv["itemType"] = "java.util.List";
    
    kvArray.append(kv);
    
    // 发送到服务端
    QString jsonStr = QJsonDocument(kvArray).toJson(QJsonDocument::Compact);
    m_httpSender->sendCommand("writeSpecialSetting", jsonStr);
    
    qInfo() << "PLC地址配置已上报到服务端";
}
