#include "../client.hpp"

int main() {
	Client client;
	int i = 0;

	while (!client.isConnected() && i++ < 3) {
		std::cout << "Attempting to connect to server..." << std::endl;
		try {
			client.connect();
		} catch (const std::exception& e) {
			std::cerr << "Error: " << e.what() << std::endl;
			if (i < 3) {
				std::cout << "Retrying in 5 seconds..." << std::endl;
				sleep(5);
			}
		}
	}
	if (client.isConnected()) {
		std::cout << "Connected to server." << std::endl;
	} else {
		std::cerr << "Failed to connect to server." << std::endl;
		return 1;
	}
	client.defineAction("OK", [](const std::string& args) {
		std::cout << "[OK] " << args << std::endl;
	});
	client.defineAction("ERR", [](const std::string& args) {
		std::cerr << "[ERR] " << args << std::endl;
	});
	client.defineAction("EVT", [](const std::string& args) {
		std::cout << "[EVT] " << args << std::endl;
	});

	client.enableKeyboard();
	while (client.update()) {}
	std::cout << "Disconnected from server." << std::endl;
	return 0;
}
