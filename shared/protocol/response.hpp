#ifndef RESPONSE_HPP
# define RESPONSE_HPP

//##############################################################################
//#                                STD INCLUDES                                #
//##############################################################################

# include <string>

//##############################################################################
//#                                  INCLUDES                                  #
//##############################################################################

# include "errors.hpp"
# include "success.hpp"

//##############################################################################
//#                                  STRUCTS                                   #
//##############################################################################

struct Response {
	bool		isOk = true;
	TapOk		okType = TapOk::PLAIN;
	TapError	error = TapError::BAD_REQUEST;
	std::string	payload;
	bool		close = false;

	/**
	 * Builds the response of a success.
	 *
	 * @param type    kind of success
	 * @param payload text added after the kind
	 * @return        a success response
	 */
	static Response success(TapOk type = TapOk::PLAIN, const std::string& payload = "") {
		Response r;
		r.isOk = true;
		r.okType = type;
		r.payload = payload;
		return r;
	}

	/** Builds the response of an error. */
	static Response failure(TapError e) {
		Response r;
		r.isOk = false;
		r.error = e;
		return r;
	}

	/** Builds the protocol line of the response (without the line end). */
	std::string line() const {
		return isOk ? tapOkLine(okType, payload) : tapErrorLine(error);
	}
};

#endif
