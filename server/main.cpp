#include "commands.hpp"

int main() {
	Server server;

	registerCommands(server);

	try {
		server.start(SERVER_PORT);
	} catch (const std::exception& e) {
		std::cerr << "Error starting server: " << e.what() << std::endl;
		return 1;
	}

	while (server.getStatus()) {
		server.updatePoll();
	}

	return 0;
}
