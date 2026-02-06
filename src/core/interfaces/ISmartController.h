#ifndef ISMARTCONTROLLER_H
#define ISMARTCONTROLLER_H

#include <QMap>
#include <QVariant>

class ISmartController {
public:
    virtual ~ISmartController() = default;
    
    virtual QMap<QString, QVariant> readData() = 0;
    virtual QMap<QString, QVariant> processData(const QMap<QString, QVariant>& inputData) = 0;
    virtual bool writeToPLC(const QMap<QString, QVariant>& controlData) = 0;
    
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isEnabled() const = 0;
};

#endif
