#include "HistoryChartWidget.h"
#include "database/DatabaseManager.h"
#include "entity/MetricIndicator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

HistoryChartWidget::HistoryChartWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadAddressList();
}

void HistoryChartWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QHBoxLayout* controlLayout = new QHBoxLayout();
    
    controlLayout->addWidget(new QLabel("地址:", this));
    m_addressCombo = new QComboBox(this);
    // 地址列表将在 loadAddressList() 中加载
    connect(m_addressCombo, &QComboBox::currentTextChanged, 
            this, &HistoryChartWidget::onAddressChanged);
    controlLayout->addWidget(m_addressCombo);
    
    controlLayout->addWidget(new QLabel("开始时间:", this));
    m_startTimeEdit = new QDateTimeEdit(this);
    m_startTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(-1));
    m_startTimeEdit->setCalendarPopup(true);
    controlLayout->addWidget(m_startTimeEdit);
    
    controlLayout->addWidget(new QLabel("结束时间:", this));
    m_endTimeEdit = new QDateTimeEdit(this);
    m_endTimeEdit->setDateTime(QDateTime::currentDateTime());
    m_endTimeEdit->setCalendarPopup(true);
    controlLayout->addWidget(m_endTimeEdit);
    
    m_loadBtn = new QPushButton("加载数据", this);
    connect(m_loadBtn, &QPushButton::clicked, this, &HistoryChartWidget::onLoadData);
    controlLayout->addWidget(m_loadBtn);
    
    controlLayout->addStretch();
    
    mainLayout->addLayout(controlLayout);
    
    m_chart = new QChart();
    m_chart->setTitle("历史数据曲线");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    
    m_series = new QLineSeries();
    m_chart->addSeries(m_series);
    
    m_axisX = new QDateTimeAxis();
    m_axisX->setFormat("MM-dd hh:mm");
    m_axisX->setTitleText("时间");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);
    
    m_axisY = new QValueAxis();
    m_axisY->setTitleText("数值");
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);
    
    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    
    mainLayout->addWidget(m_chartView);
}

void HistoryChartWidget::onLoadData() {
    // 获取实际的地址值（存储在 userData 中）
    QString address = m_addressCombo->currentData().toString();
    if (address.isEmpty()) {
        address = m_addressCombo->currentText();  // 回退到文本
    }
    
    QDateTime start = m_startTimeEdit->dateTime();
    QDateTime end = m_endTimeEdit->dateTime();
    
    loadHistoryData(address, start, end);
}

void HistoryChartWidget::loadAddressList() {
    m_addressCombo->clear();
    
    DatabaseManager* db = DatabaseManager::instance();
    QVariant indicatorValue = db->getKeyValue("METRIC_INDICATOR_KEY");
    
    if (indicatorValue.isNull() || indicatorValue.toString().isEmpty()) {
        return;
    }
    
    QString jsonStr = indicatorValue.toString();
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isArray()) {
        return;
    }
    
    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        if (!val.isObject()) {
            continue;
        }
        
        QJsonObject obj = val.toObject();
        QString address = obj.value("address").toString();
        QString name = obj.value("name").toString();
        
        if (!address.isEmpty()) {
            // 显示格式：名称 (地址)
            QString displayText = name.isEmpty() ? address : QString("%1 (%2)").arg(name).arg(address);
            m_addressCombo->addItem(displayText, address);  // 显示文本，实际值为地址
        }
    }
}

void HistoryChartWidget::onAddressChanged(const QString& address) {
    Q_UNUSED(address);
}

void HistoryChartWidget::loadHistoryData(const QString& address, 
                                         const QDateTime& start, 
                                         const QDateTime& end) {
    QList<PLCDataRecord> records = DatabaseManager::instance()->getHistoryData(
        address, start, end);
    
    m_series->clear();
    
    if (records.isEmpty()) {
        m_chart->setTitle(QString("历史数据曲线 - %1 (无数据)").arg(address));
        return;
    }
    
    double minValue = 1e9;
    double maxValue = -1e9;
    
    for (const PLCDataRecord& record : records) {
        if (record.correctedValue.isValid()) {
            double value = record.correctedValue.toDouble();
            m_series->append(record.timestamp.toMSecsSinceEpoch(), value);
            
            minValue = qMin(minValue, value);
            maxValue = qMax(maxValue, value);
        }
    }
    
    m_axisX->setRange(start, end);
    
    double range = maxValue - minValue;
    m_axisY->setRange(minValue - range * 0.1, maxValue + range * 0.1);
    
    m_chart->setTitle(QString("历史数据曲线 - %1 (%2 条记录)")
                      .arg(address)
                      .arg(records.size()));
}
