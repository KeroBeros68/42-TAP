#include "server.hpp"

int main() {
	Server server;

	// Define an action for a specific message type
	server.defineAction("TEXT", [](User& user, const std::string& message) {
		std::cout << "Received TEXT message from user " << user.id << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});

	// Start the server on port 8080
	try {
		server.start(8080);
	} catch (const std::exception& e) {
		std::cerr << "Error starting server: " << e.what() << std::endl;
		return 1;
	}

	// Main loop to update the server
	while (server.getStatus()) {
		server.updatePoll();
		// You can add a sleep or wait here to avoid busy waiting
	}

	return 0;
}