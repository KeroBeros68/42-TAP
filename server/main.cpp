#include "server.hpp"

int main() {
	Server server;

	// Define an action for a specific message type
	server.defineAction("CONNECT", [](User& user, const std::string& name) {
		if (!user.setName(name))
			return Response::failure(TapError::BAD_REQUEST);
		user.authenticate();
		std::cout << "User " << user.id() << " connected as " << user.name() << std::endl;
		return Response::success(TapOk::CONNECTED);
	});
	server.defineAction("LOOK", [](User& user, const std::string& message) {
		std::cout << "Received LOOK message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("MOVE", [](User& user, const std::string& message) {
		std::cout << "Received MOVE message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("CHAT", [](User& user, const std::string& message) {
		std::cout << "Received CHAT message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("TAKE", [](User& user, const std::string& message) {
		std::cout << "Received TAKE message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("DROP", [](User& user, const std::string& message) {
		std::cout << "Received DROP message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("INVENTORY", [](User& user, const std::string& message) {
		std::cout << "Received INVENTORY message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("TALK", [](User& user, const std::string& message) {
		std::cout << "Received TALK message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("ATTACK", [](User& user, const std::string& message) {
		std::cout << "Received ATTACK message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("STATUS", [](User& user, const std::string& message) {
		std::cout << "Received STATUS message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("QUEST", [](User& user, const std::string& message) {
		std::cout << "Received QUEST message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("QUESTS", [](User& user, const std::string& message) {
		std::cout << "Received QUESTS message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("WHO", [](User& user, const std::string& message) {
		std::cout << "Received WHO message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("GROUP", [](User& user, const std::string& message) {
		std::cout << "Received GROUP message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});
	server.defineAction("QUIT", [](User& user, const std::string& message) {
		std::cout << "Received QUIT message from user " << user.id() << std::endl;
		// Assuming the message format is "TYPE:DATA"
		std::cout << "Message content: " << message << std::endl;
		return Response::success(TapOk::DATA, "Message received");
	});

	// Start the server on the default port
	try {
		server.start(SERVER_PORT);
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