#ifndef STRESSTESTDATAGENERATOR_H
#define STRESSTESTDATAGENERATOR_H

#include <QMap>
#include <QString>
#include <QVariant>

class StressTestDataGenerator {
public:
    StressTestDataGenerator();
    
    void initialize();
    void reset();
    
    QMap<QString, QString> dataSchema() const;
    QMap<QString, QVariant> generateData();
    
private:
    double generateMockValue(const QString& address);
    
    QMap<QString, QString> m_dataSchema;
    QMap<QString, double> m_lastValues;
};

#endif
