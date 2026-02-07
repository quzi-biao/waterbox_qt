#ifndef HISTORYCHARTWIDGET_H
#define HISTORYCHARTWIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QPushButton>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

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
