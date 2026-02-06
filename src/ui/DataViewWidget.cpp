#include "DataViewWidget.h"
#include "database/DatabaseManager.h"
#include "entity/MetricIndicator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFrame>

DataViewWidget::DataViewWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadMetricIndicators();
}

void DataViewWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 顶部：更新时间（右对齐）
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    m_timeLabel = new QLabel("更新时间: --", this);
    m_timeLabel->setStyleSheet("font-size: 14px; color: #333; padding: 5px; background: transparent;");
    headerLayout->addWidget(m_timeLabel);
    mainLayout->addLayout(headerLayout);
    
    // 滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea { background: #f5f5f5; border: none; }");
    
    // 卡片容器
    m_cardsContainer = new QWidget();
    m_cardsContainer->setStyleSheet("QWidget { background: #f5f5f5; }");
    // 移除固定尺寸，让容器根据内容自动调整
    
    // 初始化一个空布局
    QGridLayout* initialLayout = new QGridLayout(m_cardsContainer);
    initialLayout->setSpacing(10);
    initialLayout->setContentsMargins(10, 10, 10, 10);
    initialLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);  // 左上对齐
    
    m_scrollArea->setWidget(m_cardsContainer);
    
    mainLayout->addWidget(m_scrollArea);
}

QWidget* DataViewWidget::createDataCard(const QString& name, const QString& value, const QString& unit, const QString& address, int width) {
    QWidget* card = new QWidget();
    card->setStyleSheet(
        "QWidget {"
        "   background-color: #fafafa;"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 6px;"
        "}"
    );
    // 固定大小，宽度根据容器计算，高度固定
    card->setFixedSize(width, 80);
    
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(2);
    cardLayout->setContentsMargins(10, 8, 10, 8);
    
    // 名称（左上角）
    QLabel* nameLabel = new QLabel(name, card);
    nameLabel->setStyleSheet("font-size: 11px; color: #666; background: transparent; border: none;");
    nameLabel->setWordWrap(false);
    cardLayout->addWidget(nameLabel, 0, Qt::AlignLeft | Qt::AlignTop);
    
    // 数值和单位（中间，大字，底部对齐）
    QHBoxLayout* valueLayout = new QHBoxLayout();
    valueLayout->setSpacing(4);
    valueLayout->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
    
    QLabel* valueLabel = new QLabel(value, card);
    valueLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #1976D2; background: transparent; border: none;");
    valueLabel->setWordWrap(false);
    valueLabel->setAlignment(Qt::AlignBottom);
    valueLayout->addWidget(valueLabel, 0, Qt::AlignBottom);
    
    if (!unit.isEmpty()) {
        QLabel* unitLabel = new QLabel(unit, card);
        unitLabel->setStyleSheet("font-size: 14px; color: #1976D2; background: transparent; border: none;");
        unitLabel->setWordWrap(false);
        unitLabel->setAlignment(Qt::AlignBottom);
        valueLayout->addWidget(unitLabel, 0, Qt::AlignBottom);
    }
    
    cardLayout->addLayout(valueLayout, 1);
    
    // 地址（右下角小字）
    QLabel* addressLabel = new QLabel(address, card);
    addressLabel->setStyleSheet("font-size: 9px; color: #999; background: transparent; border: none;");
    addressLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    cardLayout->addWidget(addressLabel, 0, Qt::AlignRight | Qt::AlignBottom);
    
    return card;
}

void DataViewWidget::loadMetricIndicators() {
    m_addressNames.clear();
    m_addressUnits.clear();
    
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
        QString unit = obj.value("unit").toString();
        
        if (!address.isEmpty()) {
            m_addressNames[address] = name.isEmpty() ? address : name;
            m_addressUnits[address] = unit;
        }
    }
}

void DataViewWidget::updateData(const QMap<QString, QVariant>& data) {
    // 更新时间
    qint64 timestamp = data.value("_timestamp", QDateTime::currentMSecsSinceEpoch()).toLongLong();
    QString currentTime = QDateTime::fromMSecsSinceEpoch(timestamp).toString("yyyy-MM-dd hh:mm:ss");
    m_timeLabel->setText("更新时间: " + currentTime);
    
    // 清空旧卡片
    if (m_cardsContainer->layout()) {
        QLayoutItem* item;
        while ((item = m_cardsContainer->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete m_cardsContainer->layout();
    }
    
    // 创建网格布局
    QGridLayout* gridLayout = new QGridLayout(m_cardsContainer);
    int spacing = 20;
    gridLayout->setHorizontalSpacing(spacing);  // 水平间距
    gridLayout->setVerticalSpacing(spacing);    // 垂直间距
    gridLayout->setContentsMargins(10, 10, 10, 10);
    
    // 根据容器宽度动态计算每行卡片数量和卡片宽度
    int containerWidth = m_scrollArea->viewport()->width();
    int maxCols = 6;  // 默认每行6个
    int margins = 20;
    
    // 计算卡片宽度：(容器宽度 - 边距 - 间距总和) / 卡片数量
    int totalSpacing = spacing * (maxCols - 1);
    int cardWidth = (containerWidth - margins - totalSpacing) / maxCols;
    
    // 确保卡片宽度合理（最小150，最大250）
    cardWidth = qBound(150, cardWidth, 250);
    
    int row = 0;
    int col = 0;
    
    // 分离BOOL类型和其他类型数据
    QList<QPair<QString, QVariant>> boolData;
    QList<QPair<QString, QVariant>> otherData;
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        // 跳过时间戳字段
        if (it.key() == "_timestamp") {
            continue;
        }
        
        if (it.value().typeId() == QMetaType::Bool) {
            boolData.append(qMakePair(it.key(), it.value()));
        } else {
            otherData.append(qMakePair(it.key(), it.value()));
        }
    }
    
    // 先显示非BOOL数据
    for (const auto& pair : otherData) {
        QString address = pair.first;
        QString name = m_addressNames.value(address, address);
        QString unit = m_addressUnits.value(address, "");
        QString value;
        
        if (pair.second.typeId() == QMetaType::Double || pair.second.typeId() == QMetaType::Float) {
            double numValue = pair.second.toDouble();
            // 异常值（小于-1000）显示为横线
            if (numValue < -1000) {
                value = "—";
            }
            // 大于100的值不显示小数点，小于等于100的显示2位小数
            else if (numValue > 100) {
                value = QString::number(qRound(numValue));
            } else {
                value = QString::number(numValue, 'f', 2);
            }
        } else {
            value = pair.second.toString();
        }
        
        QWidget* card = createDataCard(name, value, unit, address, cardWidth);
        gridLayout->addWidget(card, row, col);
        
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
    
    // 再显示BOOL数据
    for (const auto& pair : boolData) {
        QString address = pair.first;
        QString name = m_addressNames.value(address, address);
        QString unit = m_addressUnits.value(address, "");
        QString value = pair.second.toBool() ? "开" : "关";
        
        QWidget* card = createDataCard(name, value, unit, address, cardWidth);
        gridLayout->addWidget(card, row, col);
        
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
    
    // 添加弹性空间
    gridLayout->setRowStretch(row + 1, 1);
    gridLayout->setColumnStretch(maxCols, 1);
    
    // 强制更新布局
    m_cardsContainer->updateGeometry();
    m_scrollArea->updateGeometry();
    
}
