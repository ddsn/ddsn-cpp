#ifndef DDSN_PEER_MESSAGES_H
#define DDSN_PEER_MESSAGES_H

#include "../foreign_peer.h"
#include "../local_peer.h"

namespace ddsn {
	class foreign_peer;
	class peer_connection;

	class peer_message {
	public:
		static peer_message *create_message(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection, const std::string &first_line);

		peer_message(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection);
		virtual ~peer_message();

		// called immediately after receiving the first line
		virtual void first_action(int &type, size_t &expected_size) = 0;

		// provides this message with a string
		virtual void feed(const std::string &line, int &type, size_t &expected_size) = 0;

		// provides this message with a byte array
		virtual void feed(const char *data, size_t size, int &type, size_t &expected_size) = 0;
	protected:
		foreign_peer *foreign_peer_;
		local_peer &local_peer_;
		peer_connection &connection_;
	};

	namespace peer_messages {
		// TODO: Implement some ;)
	}
}

#endif