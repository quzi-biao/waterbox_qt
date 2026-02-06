#ifndef PLCADDRESSCONFIGWIDGET_H
#define PLCADDRESSCONFIGWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QList>
#include "entity/MetricIndicator.h"

class HttpSender;

class PLCAddressConfigWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit PLCAddressConfigWidget(QWidget* parent = nullptr);
    
    void setHttpSender(HttpSender* sender);
    void loadConfig();
    void saveConfig();
    
public slots:
    void reloadConfig();
    
private slots:
    void onAddRow();
    void onDeleteRow();
    void onSaveConfig();
    
private:
    void setupUI();
    void loadIndicators();
    void saveIndicators();
    void uploadToServer();
    
    QTableWidget* m_table;
    QPushButton* m_addBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_saveBtn;
    
    HttpSender* m_httpSender;
    QList<MetricIndicator> m_indicators;
};

#endif
