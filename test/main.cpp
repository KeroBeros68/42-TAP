
#include <QtWidgets>

#include "globals.hpp"
#include "gui/connection_window/connection_window.hpp"
#include <QPushButton>
#include <QVBoxLayout>

int main(int argc, char *argv[])
{
	QApplication		app(argc, argv); // Initialization of the QT engine
	TAPConnectionWindow	window; // Creation d'un widget fenetre

	// Window initialization
	window.resize(WINDOW_WIDTH, WINDOW_HEIGHT); // Dimensions
	window.setWindowTitle("42 TAP - RFC GUI"); // Title
	std::string str = "background-color: ";
	str += BG_COLOR;
	window.setStyleSheet(str.c_str()); // Bg color
	QPushButton *button = new QPushButton("&Download", &window); // Create button
	window.connect(button, &QPushButton::clicked, &window, &TAPConnectionWindow::setRemoteAddress); // Set button usage

	// Start showing the window
	window.show();

	return app.exec();
}
