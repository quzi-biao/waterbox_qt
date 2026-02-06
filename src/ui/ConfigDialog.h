#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTableWidget>

class ConfigDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit ConfigDialog(QWidget* parent = nullptr);
    
private slots:
    void onAccept();
    void onAddAddress();
    void onRemoveAddress();
    
private:
    void setupUI();
    void loadConfig();
    void saveConfig();
    
    QLineEdit* m_plcHostEdit;
    QSpinBox* m_plcPortSpin;
    QComboBox* m_protocolCombo;
    
    QLineEdit* m_reportUrlEdit;
    QComboBox* m_reportProtocolCombo;
    
    QSpinBox* m_collectIntervalSpin;
    QSpinBox* m_reportIntervalSpin;
    
    QLineEdit* m_ctrlPressAddrEdit;
    QLineEdit* m_endPressAddrEdit;
    QLineEdit* m_flowAddrEdit;
    
    QTableWidget* m_addressTable;
};

#endif
