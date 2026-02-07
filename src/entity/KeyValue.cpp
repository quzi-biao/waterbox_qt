#include "KeyValue.h"
#include <QVariant>

KeyValue::KeyValue()
    : m_id(0) {
}

QJsonObject KeyValue::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["itemKey"] = m_itemKey;
    obj["itemValue"] = m_itemValue;
    obj["itemType"] = m_itemType;
    return obj;
}

KeyValue KeyValue::fromJson(const QJsonObject& json) {
    KeyValue kv;
    kv.setId(json.value("id").toVariant().toLongLong());
    kv.setItemKey(json.value("itemKey").toString());
    kv.setItemValue(json.value("itemValue").toString());
    kv.setItemType(json.value("itemType").toString());
    return kv;
}
