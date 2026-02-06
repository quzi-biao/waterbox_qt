#ifndef SERVICERESPONSEHANDLE_H
#define SERVICERESPONSEHANDLE_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QByteArray>

class HttpSender;
class CommandHandler;

class ServiceResponseHandle : public QObject {
    Q_OBJECT
    
public:
    explicit ServiceResponseHandle(QObject* parent = nullptr);
    ~ServiceResponseHandle();
    
    void initialize(HttpSender* sender);
    
    static void addResponse(const QByteArray& data);
    
    CommandHandler* commandHandler() const { return m_commandHandler; }
    
private slots:
    void processResponses();
    
private:
    void processResponse(const QString& data);
    
    static QQueue<QString> s_responses;
    
    QTimer* m_timer;
    HttpSender* m_sender;
    CommandHandler* m_commandHandler;
};

#endif
