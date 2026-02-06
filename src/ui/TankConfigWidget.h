#ifndef TANKCONFIGWIDGET_H
#define TANKCONFIGWIDGET_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>

class TankConfigWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit TankConfigWidget(QWidget* parent = nullptr);
    
    void loadConfig();
    void saveConfig();
    
private:
    void setupUI();
    
    QDoubleSpinBox* m_boxLongSpin;
    QDoubleSpinBox* m_boxWideSpin;
    QDoubleSpinBox* m_boxHighSpin;
    QSpinBox* m_boxNumSpin;
};

#endif
