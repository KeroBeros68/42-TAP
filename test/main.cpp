
#include <QtWidgets>

#include "globals.hpp"

int main(int argc, char *argv[])
{
	QApplication	app(argc, argv); // Initialization of the QT engine
	QWidget			window; // Creation d'un widget fenetre

	// Window initialization
	window.resize(WINDOW_WIDTH, WINDOW_HEIGHT); // Dimensions
	window.setWindowTitle("42 TAP - RFC GUI"); // Title
	window.setStyleSheet("background-color: #fefae0");

	// Start showing the window
	window.show();

	return app.exec();
}
