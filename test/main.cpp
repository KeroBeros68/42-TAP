#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include "globals.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("42TAP RFC GUI");
    window.resize(WINDOW_WIDTH, WINDOW_HEIGHT);
    while (true)
    {
        printf("%d\n")
    }
    

    window.show();
    return app.exec();
}
