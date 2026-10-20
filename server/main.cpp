//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "../shared/protocol/event.hpp"
# include "../shared/logging/logger.hpp"
# include "../shared/system/loadenv.hpp"
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
	loadDefaultEnv();
	Logger::instance().setMinLevel(logLevelFromName(getEnv(ENV_LOG_LEVEL), LogLevel::Debug));

	try {
		Server server;
		Game game;

		registerCommands(server, game);
		server.setOnDisconnect([&game](User& user) {
			game.removePlayer(user.id());
		});

		server.start(static_cast<size_t>(getEnvInt(ENV_PORT, SERVER_PORT, MIN_PORT, MAX_PORT)));

		while (server.getStatus()) {
			try {
				server.updatePoll();
			} catch (const std::exception& e) {
				LOG_ERROR("network turn failed", {"error", e.what()});
			}
			try {
				game.tick(std::chrono::steady_clock::now());
				for (const Event& event : game.takeEvents())
					server.sendToArray(event.line, event.players_ids);
				server.flushAll();
			} catch (const std::exception& e) {
				LOG_ERROR("game turn failed", {"error", e.what()});
			}
		}
		LOG_INFO("server stopped");
	} catch (const std::exception& e) {
		LOG_ERROR("fatal error", {"error", e.what()});
		return 1;
	}

	return 0;
}
