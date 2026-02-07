#include "StressTestDataGenerator.h"
#include "database/DatabaseManager.h"
#include "entity/MetricIndicator.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDebug>

StressTestDataGenerator::StressTestDataGenerator() {
}

void StressTestDataGenerator::initialize() {
    // 创建模拟水务场景的指标配置
    QJsonArray mockArray;
    
    auto addIndicator = [&](qint64 id, const QString& address, int dataType,
                            const QString& name, const QString& unit) {
        QJsonObject obj;
        obj["id"] = id;
        obj["address"] = address;
        obj["dataType"] = dataType;
        obj["writable"] = false;
        obj["name"] = name;
        obj["unit"] = unit;
        mockArray.append(obj);
    };
    
    // 水务场景典型 PLC 地址
    addIndicator(90001, "V100", MetricIndicatorDataType::FLOAT32, "出水压力", "MPa");
    addIndicator(90002, "V104", MetricIndicatorDataType::FLOAT32, "进水压力", "MPa");
    addIndicator(90003, "V108", MetricIndicatorDataType::FLOAT32, "瞬时流量", "m³/h");
    addIndicator(90004, "V112", MetricIndicatorDataType::FLOAT32, "累计流量", "m³");
    addIndicator(90005, "V116", MetricIndicatorDataType::FLOAT32, "水箱液位", "m");
    addIndicator(90006, "V120", MetricIndicatorDataType::FLOAT32, "泵电流", "A");
    addIndicator(90007, "V124", MetricIndicatorDataType::FLOAT32, "泵频率", "Hz");
    addIndicator(90008, "V128", MetricIndicatorDataType::FLOAT32, "末端压力", "MPa");
    addIndicator(90009, "V132", MetricIndicatorDataType::INT16,   "泵运行状态", "");
    addIndicator(90010, "V134", MetricIndicatorDataType::FLOAT32, "水温", "℃");
    
    // 保存到数据库 METRIC_INDICATOR_KEY_MOCK
    QString jsonStr = QJsonDocument(mockArray).toJson(QJsonDocument::Compact);
    DatabaseManager::instance()->saveKeyValue("METRIC_INDICATOR_KEY_MOCK", jsonStr);
    
    // 加载到 dataSchema
    m_dataSchema.clear();
    for (const QJsonValue& val : mockArray) {
        QJsonObject obj = val.toObject();
        m_dataSchema[obj["address"].toString()] = QString::number(obj["dataType"].toInt());
    }
    
    qInfo() << "压测模拟指标已初始化:" << m_dataSchema.size() << "个";
}

void StressTestDataGenerator::reset() {
    m_lastValues.clear();
}

QMap<QString, QString> StressTestDataGenerator::dataSchema() const {
    return m_dataSchema;
}

QMap<QString, QVariant> StressTestDataGenerator::generateData() {
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    QMap<QString, QVariant> data;
    
    for (auto it = m_dataSchema.constBegin(); it != m_dataSchema.constEnd(); ++it) {
        QString address = it.key();
        double value = generateMockValue(address);
        data[address] = value;
        
        // 保存到数据库，标记为压测数据
        DatabaseManager::instance()->saveData(address, value, value, true);
    }
    
    data["_timestamp"] = timestamp;
    data["_stress_test"] = true;
    
    return data;
}

double StressTestDataGenerator::generateMockValue(const QString& address) {
    QRandomGenerator* rng = QRandomGenerator::global();
    double last = m_lastValues.value(address, -1.0);
    double value = 0.0;
    
    // 根据地址生成符合水务场景的值
    if (address == "V100") {
        // 出水压力: 0.25~0.45 MPa
        value = (last < 0) ? 0.35 : last + (rng->generateDouble() - 0.5) * 0.02;
        value = qBound(0.25, value, 0.45);
    } else if (address == "V104") {
        // 进水压力: 0.15~0.30 MPa
        value = (last < 0) ? 0.22 : last + (rng->generateDouble() - 0.5) * 0.01;
        value = qBound(0.15, value, 0.30);
    } else if (address == "V108") {
        // 瞬时流量: 10~120 m³/h
        value = (last < 0) ? 60.0 : last + (rng->generateDouble() - 0.5) * 10.0;
        value = qBound(10.0, value, 120.0);
    } else if (address == "V112") {
        // 累计流量: 递增
        value = (last < 0) ? 10000.0 : last + rng->generateDouble() * 2.0;
    } else if (address == "V116") {
        // 水箱液位: 1.0~3.5 m
        value = (last < 0) ? 2.5 : last + (rng->generateDouble() - 0.5) * 0.1;
        value = qBound(1.0, value, 3.5);
    } else if (address == "V120") {
        // 泵电流: 15~45 A
        value = (last < 0) ? 30.0 : last + (rng->generateDouble() - 0.5) * 2.0;
        value = qBound(15.0, value, 45.0);
    } else if (address == "V124") {
        // 泵频率: 30~50 Hz
        value = (last < 0) ? 42.0 : last + (rng->generateDouble() - 0.5) * 1.0;
        value = qBound(30.0, value, 50.0);
    } else if (address == "V128") {
        // 末端压力: 0.15~0.35 MPa
        value = (last < 0) ? 0.25 : last + (rng->generateDouble() - 0.5) * 0.015;
        value = qBound(0.15, value, 0.35);
    } else if (address == "V132") {
        // 泵运行状态: 0 或 1
        value = (rng->generateDouble() > 0.05) ? 1.0 : 0.0;
    } else if (address == "V134") {
        // 水温: 15~28 ℃
        value = (last < 0) ? 22.0 : last + (rng->generateDouble() - 0.5) * 0.3;
        value = qBound(15.0, value, 28.0);
    } else {
        value = rng->generateDouble() * 100.0;
    }
    
    m_lastValues[address] = value;
    return value;
}
