#include <QtWidgets>
#include <QPushButton>
#include <QVBoxLayout>
#include <csignal>

#include "src/windows/windows.hpp"
#include "../client.hpp"
#include "globals.hpp"
#include <stdlib.h>

int main(int argc, char *argv[])
{
	QApplication		app(argc, argv); // Initialization of the QT engine
	app.setWindowIcon(QIcon(":/src/icon.ico"));

	// Ctrl+C signal handling for clean exit
	std::signal(SIGINT, [](int) {
		// Add the app.quit method to the QT event queue
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
    });

	// // Connection window
	// TAPConnectionWindow	connection_window = TAPConnectionWindow(
	// 	"42 TAP GUI",
	// 	WINDOW_WIDTH,
	// 	WINDOW_HEIGHT
	// );
	// connection_window.show();

	// // Error window
	// TAPErrorWindow error_window = TAPErrorWindow();
	// error_window.setErrorMessage("Test error :-)");
	// error_window.show();

	// Game window
	TAPGameWindow game_window = TAPGameWindow();
	game_window.show();
	game_window.update_total_players_label(42);
	game_window.update_players_in_room_label(21);
    std::map<std::string, std::string> ex = {
        {"north", "Datacenter hall"},
        {"south", "Napo office"},
		{"east", "Gambrinus"},
		{"west", "KM0"}
	};
	game_window.update_available_exits(ex);
	game_window.update_room_name("42 datacenter");
	game_window.update_room_description("A silent and strange feeling fills this place. It looks... empty. As you slowly walk here, a thick fog starts to form. You feel observed.");

	try {
		app.exec;
	}
	catch (const std::exception& e) {
		app.quit();
	}
}
