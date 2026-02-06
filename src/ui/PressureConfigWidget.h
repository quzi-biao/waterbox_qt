#ifndef PRESSURECONFIGWIDGET_H
#define PRESSURECONFIGWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>

class PressureConfigWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit PressureConfigWidget(QWidget* parent = nullptr);
    
    void loadConfig();
    void saveConfig();
    
private:
    void setupUI();
    
    QCheckBox* m_openPressCtrlCheck;
    QDoubleSpinBox* m_minPressSpin;
    QDoubleSpinBox* m_defaultPressSpin;
    QDoubleSpinBox* m_maxPressSpin;
    QDoubleSpinBox* m_pressIncreaseStepSpin;
    QSpinBox* m_pressIncreaseIntervalSpin;
    QLineEdit* m_ctrlPressPlcAddressEdit;
    QLineEdit* m_endPressAddressEdit;
    QLineEdit* m_endPressDeviceCodeEdit;
    QCheckBox* m_endPressReimCheck;
    QDoubleSpinBox* m_endPressReimRateSpin;
    QDoubleSpinBox* m_endPressStandardSpin;
    QSpinBox* m_endPressAvgTimeSpin;
    QSpinBox* m_pressCtrlTypeSpin;
};

#endif
