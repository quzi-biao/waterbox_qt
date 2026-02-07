#include "MetricIndicator.h"
#include <QVariant>

MetricIndicator::MetricIndicator()
    : m_id(0),
      m_pumproomId(0),
      m_writable(false),
      m_dataType(MetricIndicatorDataType::INT16) {
}

QJsonObject MetricIndicator::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["pumproomId"] = m_pumproomId;
    obj["address"] = m_address;
    obj["writable"] = m_writable;
    obj["dataType"] = m_dataType;
    obj["name"] = m_name;
    obj["unit"] = m_unit;
    return obj;
}

MetricIndicator MetricIndicator::fromJson(const QJsonObject& json) {
    MetricIndicator indicator;
    indicator.setId(json.value("id").toVariant().toLongLong());
    indicator.setPumproomId(json.value("pumproomId").toVariant().toLongLong());
    indicator.setAddress(json.value("address").toString());
    indicator.setWritable(json.value("writable").toBool());
    indicator.setDataType(json.value("dataType").toInt());
    indicator.setName(json.value("name").toString());
    indicator.setUnit(json.value("unit").toString());
    return indicator;
}
