#ifndef METRICINDICATOR_H
#define METRICINDICATOR_H

#include <QString>
#include <QJsonObject>

// 数据类型枚举（与 Java 后端保持一致）
namespace MetricIndicatorDataType {
    const int INT16 = 0;
    const int INT32 = 1;
    const int UINT16 = 2;
    const int UINT32 = 3;
    const int FLOAT32 = 4;
    const int FLOAT64 = 5;
    const int BYTE = 6;
    const int BOOL = 7;
}

class MetricIndicator {
public:
    MetricIndicator();
    
    qint64 id() const { return m_id; }
    void setId(qint64 id) { m_id = id; }
    
    qint64 pumproomId() const { return m_pumproomId; }
    void setPumproomId(qint64 pumproomId) { m_pumproomId = pumproomId; }
    
    QString address() const { return m_address; }
    void setAddress(const QString& address) { m_address = address; }
    
    bool writable() const { return m_writable; }
    void setWritable(bool writable) { m_writable = writable; }
    
    int dataType() const { return m_dataType; }
    void setDataType(int dataType) { m_dataType = dataType; }
    
    QString unit() const { return m_unit; }
    void setUnit(const QString& unit) { m_unit = unit; }
    
    QJsonObject toJson() const;
    static MetricIndicator fromJson(const QJsonObject& json);
    
private:
    qint64 m_id;
    qint64 m_pumproomId;
    QString m_address;
    bool m_writable;
    int m_dataType;
    QString m_unit;
};

#endif
