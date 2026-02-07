#ifndef DATAVIEWWIDGET_H
#define DATAVIEWWIDGET_H

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QMap>
#include <QVariant>

class DataViewWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit DataViewWidget(QWidget* parent = nullptr);
    
    void updateData(const QMap<QString, QVariant>& data);
    void loadMetricIndicators();
    
private:
    void setupUI();
    void loadMetricIndicatorsFromKey(const QString& key);
    QWidget* createDataCard(const QString& name, const QString& value, const QString& unit, const QString& address, int width = 180);
    
    QScrollArea* m_scrollArea;
    QWidget* m_cardsContainer;
    QLabel* m_timeLabel;
    QMap<QString, QString> m_addressNames;  // 地址 -> 名称
    QMap<QString, QString> m_addressUnits;  // 地址 -> 单位
};

#endif
