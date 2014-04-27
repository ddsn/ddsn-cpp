#include "peer_messages.h"
#include "definitions.h"

#include <boost/lexical_cast.hpp>

using namespace ddsn;
using namespace std;

peer_message *peer_message::create_message(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection, const string &first_line) {
	if (first_line == "HELLO") {
		return new peer_hello(local_peer, foreign_peer, connection);
	} else if (first_line == "PROVE IDENTITY") {
		return new peer_hello(local_peer, foreign_peer, connection);
	} else if (first_line == "VERIFY IDENTITY") {
		return new peer_hello(local_peer, foreign_peer, connection);
	}
	return nullptr;
}

peer_message::peer_message(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection) :
	local_peer_(local_peer), foreign_peer_(foreign_peer), connection_(connection) {

}

peer_message::~peer_message() {

}

// HELLO

peer_hello::peer_hello(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection) :
	peer_message(local_peer, foreign_peer, connection), state_(0) {

}

peer_hello::~peer_hello() {

}

void peer_hello::first_action(int &type, size_t &expected_size) {
	type = DDSN_PEER_MESSAGE_TYPE_BYTES;
	expected_size = 32;
}

void peer_hello::feed(const std::string &line, int &type, size_t &expected_size) {
	if (state_ == 0) {
		if (line != "-----BEGIN RSA PUBLIC KEY-----") {
			public_key_ += line + "\n";

			type = DDSN_PEER_MESSAGE_TYPE_ERROR;
			return;
		} else {
			state_ = 1;
			type = DDSN_PEER_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 1) {
		if (line == "-----END RSA PRIVATE KEY-----") {
			foreign_peer_->set_public_key(public_key_);

			connection_.send("PROVE IDENTITY\n"
				"Sign this random number: " + boost::lexical_cast<std::string>(53543) + "\n"); // TODO: Make it random

			type = DDSN_PEER_MESSAGE_TYPE_END;
		} else {
			if (line.length() == 64) {
				public_key_ += line + "\n";

				type = DDSN_PEER_MESSAGE_TYPE_STRING;
			} else {
				type = DDSN_PEER_MESSAGE_TYPE_ERROR;
			}
		}
	}
}

void peer_hello::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	if (size == 32) {
		peer_id foreign_id((unsigned char *)data);

		foreign_peer_ = new foreign_peer();
		foreign_peer_->set_id(foreign_id);

		connection_.set_foreign_peer(foreign_peer_);

		type = DDSN_PEER_MESSAGE_TYPE_STRING;
	}
}

// PROVE IDENTITY

peer_prove_identity::peer_prove_identity(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection) :
	peer_message(local_peer, foreign_peer, connection) {

}

peer_prove_identity::~peer_prove_identity() {

}

void peer_prove_identity::first_action(int &type, size_t &expected_size) {
}

void peer_prove_identity::feed(const std::string &line, int &type, size_t &expected_size) {
}

void peer_prove_identity::feed(const char *data, size_t size, int &type, size_t &expected_size) {
}

// VERIFY IDENTITY

peer_verify_identity::peer_verify_identity(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection) :
	peer_message(local_peer, foreign_peer, connection) {

}

peer_verify_identity::~peer_verify_identity() {

}

void peer_verify_identity::first_action(int &type, size_t &expected_size) {
}

void peer_verify_identity::feed(const std::string &line, int &type, size_t &expected_size) {
}

void peer_verify_identity::feed(const char *data, size_t size, int &type, size_t &expected_size) {
}