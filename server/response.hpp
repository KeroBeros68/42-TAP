#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <string>

# include "errors.hpp"
# include "success.hpp"

// What an action returns: either a success (TapOk + optional payload) or an error (TapError).
// The server turns it into a protocol line and sends it to the client.
struct Response {
	bool		isOk = true;
	TapOk		okType = TapOk::PLAIN;
	TapError	error = TapError::BAD_REQUEST;
	std::string	payload;
	bool		close = false;	// ask the server to close the connection after sending

	static Response success(TapOk type = TapOk::PLAIN, const std::string& payload = "") {
		Response r;
		r.isOk = true;
		r.okType = type;
		r.payload = payload;
		return r;
	}

	static Response failure(TapError e) {
		Response r;
		r.isOk = false;
		r.error = e;
		return r;
	}

	// Full protocol line, without the trailing '\n'
	std::string line() const {
		return isOk ? tapOkLine(okType, payload) : tapErrorLine(error);
	}
};

#endif
