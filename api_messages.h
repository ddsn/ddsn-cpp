#ifndef DDSN_API_MESSAGES_H
#define DDSN_API_MESSAGES_H

#include "api_server.h"
#include "local_peer.h"

#include <string>

namespace ddsn {
	class api_connection;

	class api_message {
	public:
		static api_message *create_message(const std::string &first_line);

		api_message(local_peer &local_peer, api_connection &connection);
		virtual ~api_message();

		virtual void feed(const std::string &line, int &type, size_t &expected_size) = 0;
		virtual void feed(const char *data, size_t size, int &type, size_t &expected_size) = 0;
	private:
		local_peer &local_peer_;
		api_connection &connection_;
	};
}

#endif