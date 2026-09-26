
#include <QtWidgets>

#include "globals.hpp"
#include "gui/windows/windows.hpp"
#include <QPushButton>
#include <QVBoxLayout>

int main(int argc, char *argv[])
{
	QApplication		app(argc, argv); // Initialization of the QT engine
	TAPConnectionWindow	connection_window = TAPConnectionWindow(
		"42 TAP GUI",
		WINDOW_WIDTH,
		WINDOW_HEIGHT
	);

	connection_window.show();
	return app.exec();
}
