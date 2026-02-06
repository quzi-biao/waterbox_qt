#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QThread>

class PLCClient;
class DataCollector;
class DataSender;
class PressureController;
class ConfigDialog;
class DataViewWidget;
class HistoryChartWidget;
class PLCSimulator;
class HttpSender;
class SystemMonitor;
class ServiceResponseHandle;
class SystemConfigWidget;
class PLCAddressConfigWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
private slots:
    void onConnectPLC();
    void onDisconnectPLC();
    void onStartSystem();
    void onStopSystem();
    void updateStatusBar();
    void onDataCollected(const QMap<QString, QVariant>& data);
    void autoConnectPLC();
    void checkPLCConnection();
    void onConfigUpdated();
    
private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void initializeSystem();
    
    QTabWidget* m_tabWidget;
    DataViewWidget* m_dataView;
    HistoryChartWidget* m_historyChart;
    SystemConfigWidget* m_systemConfigWidget;
    PLCAddressConfigWidget* m_plcAddressConfigWidget;
    
    QPushButton* m_reconnectBtn;
    
    QLabel* m_statusLabel;
    QLabel* m_plcStatusLabel;
    QLabel* m_plcConnectionLabel;
    QLabel* m_dataCountLabel;
    
    QTimer* m_statusTimer;
    QTimer* m_plcCheckTimer;
    
    PLCClient* m_plcClient;
    PLCSimulator* m_simulator;
    DataCollector* m_collector;
    DataSender* m_sender;
    PressureController* m_pressureController;
    HttpSender* m_httpSender;
    SystemMonitor* m_systemMonitor;
    ServiceResponseHandle* m_responseHandle;
    
    // 工作线程
    QThread* m_collectorThread;
    QThread* m_senderThread;
    
    bool m_systemRunning;
    bool m_simulatorMode;
    bool m_pressureTestMode;
};

#endif
