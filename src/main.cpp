#include <QApplication>
#include "app/appwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Synera Starter");
    app.setApplicationVersion("1.0");

    AppWindow window;
    window.setWindowTitle("Synera - Starter");
    window.showFullScreen();

    return app.exec();
}
