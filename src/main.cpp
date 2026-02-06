#include "ui/MainWindow.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    app.setApplicationName("WaterBox Qt");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("WaterAI");
    
    qInfo() << "Starting WaterBox Qt Application";
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
