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

	// Network client (reused from the CLI client).
	//  Not connected yet : no connect() call, no event loop integration.
	// Client client = Client();
	
	// // Connection window
	// TAPConnectionWindow	connection_window = TAPConnectionWindow(
	// 	"42 TAP GUI",
	// 	WINDOW_WIDTH,
	// 	WINDOW_HEIGHT,
	// 	client
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
	game_window.update_room_name("42 datacenter");
	game_window.update_room_description("A silent and strange feeling fills this place. It looks... empty. As you slowly walk here, a thick fog starts to form. You feel observed.");
	game_window.update_available_exits({
        {"north", "Datacenter hall"},
        {"south", "Napo office"},
		{"east", "Gambrinus"},
		{"west", "KM0"}
	});
	game_window.add_global_message({
		{"author", "bob"},
		{"content", "hey there"}
	});
	game_window.add_global_message({
		{"author", "alice"},
		{"content", "hey, welcome !"}
	});
	game_window.add_room_message({
		{"author", "alice"},
		{"content", "hey, welcome !"}
	});

    // Debug items : ground and inventory
    game_window.update_ground_items(std::map<std::string, std::string> {
        {"item.kevin_sword#1", "kevin sword"},
        {"item.kevin_bouclier#2", "kevin bouclier"},
    });
    game_window.update_inventory(std::map<std::string, std::string> {
        {"item.herbs#3", "herbs"},
    });
    game_window.update_npcs(std::map<std::string, std::string> {
        {"npc.kevin_bertrand_cheronnet", "Kevin Bertrand Cheronnet"},
        {"npc.laurent", "Laurent"},
    });
	game_window.update_quests(<%
		{
			{"quest_id", "fetch_shield"},
			{"status", "active"},
			{"progress", "1/3"}
		},
		{
			{"quest_id", "slay_dragon"},
			{"status", "active"},
			{"progress", "0/1"}
		}
	%>);


	#include <stdlib.h>
    try {
		app.exec();
	}
	catch (const std::exception& e) {
		app.quit();
	}
}
