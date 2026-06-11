#include <QApplication>
#include "Application/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("UnknownTxd");
    
    MainWindow window;
    window.show();
    
    if (argc > 1)
        window.OpenFile(argv[1]);
        
    return app.exec();
}
