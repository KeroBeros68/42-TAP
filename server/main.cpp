#include "commands.hpp"

int main() {
	try {
		Server server;
		Game game;

		registerCommands(server, game);
		server.setOnDisconnect([&game](User& user) {
			game.removePlayer(user.id());
		});

		server.start(SERVER_PORT);

		while (server.getStatus()) {
			server.updatePoll();
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
