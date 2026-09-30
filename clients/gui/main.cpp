
#include <QtWidgets>

#include "globals.hpp"
#include "src/windows/windows.hpp"
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

	TAPErrorWindow error_window = TAPErrorWindow();
	error_window.setErrorMessage("Test error :-)");

	error_window.show();
	connection_window.show();
	TAPGameWindow game_window = TAPGameWindow();
	game_window.show();
	return app.exec();
}
