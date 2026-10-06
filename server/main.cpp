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
		LOG_INFO("server stopped");
	} catch (const std::exception& e) {
		LOG_ERROR("fatal error", {"error", e.what()});
		return 1;
	}

	return 0;
}
