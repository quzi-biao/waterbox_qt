#ifndef HISTORYCHARTWIDGET_H
#define HISTORYCHARTWIDGET_H

#include <QWidget>
#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QChart>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QPushButton>

class HistoryChartWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit HistoryChartWidget(QWidget* parent = nullptr);
    
    void loadAddressList();
    
private slots:
    void onLoadData();
    void onAddressChanged(const QString& address);
    
private:
    void setupUI();
    void loadHistoryData(const QString& address, const QDateTime& start, const QDateTime& end);
    
    QChartView* m_chartView;
    QChart* m_chart;
    QLineSeries* m_series;
    QDateTimeAxis* m_axisX;
    QValueAxis* m_axisY;
    
    QComboBox* m_addressCombo;
    QDateTimeEdit* m_startTimeEdit;
    QDateTimeEdit* m_endTimeEdit;
    QPushButton* m_loadBtn;
};

#endif
