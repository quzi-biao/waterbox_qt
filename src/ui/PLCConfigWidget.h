#ifndef PLCCONFIGWIDGET_H
#define PLCCONFIGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>

class PLCConfigWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit PLCConfigWidget(QWidget* parent = nullptr);
    
    void loadConfig();
    void saveConfig();
    
private:
    void setupUI();
    
    QCheckBox* m_plcServerCheck;
    QLineEdit* m_plcLocalHostEdit;
    QSpinBox* m_plcPortSpin;
    QComboBox* m_plcProtocolCombo;
    QCheckBox* m_plcSimulateCheck;
    QCheckBox* m_plcCtrlCheck;
    QLineEdit* m_plcValveReadAddressEdit;
    QLineEdit* m_plcValveWriteAddressEdit;
};

#endif
