//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../shared/event.hpp"
# include "protocol/commands.hpp"

//##############################################################################
//#                                 FUNCTIONS                                  #
//##############################################################################

/**
 * Runs the server: defines the commands, then loops until a signal stops it.
 * Each turn of the loop handles the network, lets the time pass in the game, and delivers the events.
 *
 * @return 0, or 1 after a fatal error
 */
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
			game.tick(std::chrono::steady_clock::now());
			for (const Event& event : game.takeEvents())
				server.sendToArray(event.line, event.players_ids);
		}
		LOG_INFO("server stopped");
	} catch (const std::exception& e) {
		LOG_ERROR("fatal error", {"error", e.what()});
		return 1;
	}

	return 0;
}
