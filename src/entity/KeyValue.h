#ifndef KEYVALUE_H
#define KEYVALUE_H

#include <QString>
#include <QJsonObject>

class KeyValue {
public:
    KeyValue();
    
    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }
    
    QString itemKey() const { return m_itemKey; }
    void setItemKey(const QString& key) { m_itemKey = key; }
    
    QString itemValue() const { return m_itemValue; }
    void setItemValue(const QString& value) { m_itemValue = value; }
    
    QString itemType() const { return m_itemType; }
    void setItemType(const QString& type) { m_itemType = type; }
    
    QJsonObject toJson() const;
    static KeyValue fromJson(const QJsonObject& json);
    
private:
    qint64 m_id;
    QString m_itemKey;
    QString m_itemValue;
    QString m_itemType;
};

#endif
