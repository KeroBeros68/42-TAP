#include <QApplication>
#include <QMainWindow>
#include <QLabel>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Ma Fenêtre Qt");
    window.resize(400, 300);
    
    QLabel *label = new QLabel("Bonjour depuis Qt !", &window);
    QLabel *label2 = new QLabel("Bonjour depuis Qttttttttttttt !", &window);
    window.setCentralWidget(label);
    window.setCentralWidget(label2);

    window.show();
    return app.exec();
}
