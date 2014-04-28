#include "peer_messages.h"
#include "definitions.h"

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <boost/lexical_cast.hpp>

using namespace ddsn;
using namespace std;

peer_message *peer_message::create_message(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection::pointer connection, const string &first_line) {
	if (first_line == "HELLO") {
		return new peer_hello(local_peer, foreign_peer, connection);
	} else if (first_line == "PROVE IDENTITY") {
		return new peer_prove_identity(local_peer, foreign_peer, connection);
	} else if (first_line == "VERIFY IDENTITY") {
		return new peer_verify_identity(local_peer, foreign_peer, connection);
	}
	return nullptr;
}

peer_message::peer_message(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection::pointer connection) :
	local_peer_(local_peer), foreign_peer_(foreign_peer), connection_(connection) {

}

peer_message::~peer_message() {

}

// HELLO

peer_hello::peer_hello(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection::pointer connection) :
	peer_message(local_peer, foreign_peer, connection), state_(0) {

}

peer_hello::~peer_hello() {

}

void peer_hello::first_action(int &type, size_t &expected_size) {
	type = DDSN_PEER_MESSAGE_TYPE_BYTES;
	expected_size = 32;
}

void peer_hello::feed(const std::string &line, int &type, size_t &expected_size) {
	cout << line << endl;

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
		if (line == "-----END RSA PUBLIC KEY-----") {
			foreign_peer_->set_public_key_str(public_key_);
			foreign_peer_->set_verification_number(12345); // TODO: make it random

			cout << "PEER#" << connection_->id() << " sent public key pem (" << public_key_.length() << " bytes)" << endl;

			peer_prove_identity(local_peer_, foreign_peer_, connection_).send();

			type = DDSN_PEER_MESSAGE_TYPE_END;
		} else {
			public_key_ += line + "\n";

			type = DDSN_PEER_MESSAGE_TYPE_STRING;
		}
	}
}

void peer_hello::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	if (size == 32) {
		peer_id foreign_id((unsigned char *)data);

		for (int i = 0; i < 32; i++) {
			cout << foreign_id.id()[i] << " ";
		}
		cout << endl;

		cout << "PEER#" << connection_->id() << " claims id " << foreign_id.short_string() << endl;

		foreign_peer_ = new foreign_peer();
		foreign_peer_->set_id(foreign_id);

		connection_->set_foreign_peer(foreign_peer_);

		type = DDSN_PEER_MESSAGE_TYPE_STRING;
	}
}

void peer_hello::send() {
	cout << "Sending hello to PEER#" << connection_->id() << endl;

	connection_->send("HELLO\n");
	connection_->send((char *)local_peer_.id().id(), 32);

	for (int i = 0; i < 32; i++) {
		cout << (int)local_peer_.id().id()[i] << " ";
	}
	cout << endl;

	// send public key in pem format

	BIO *pri = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPublicKey(pri, local_peer_.keypair());

	size_t pri_len = BIO_pending(pri);

	char *pri_key = new char[pri_len];

	BIO_read(pri, pri_key, pri_len);

	// actually a string, but we can use byte verion of send
	connection_->send(pri_key, pri_len);

	cout << "Sent hello to PEER#" << connection_->id() << endl;
}

// PROVE IDENTITY

peer_prove_identity::peer_prove_identity(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection::pointer connection) :
	peer_message(local_peer, foreign_peer, connection) {

}

peer_prove_identity::~peer_prove_identity() {

}

void peer_prove_identity::first_action(int &type, size_t &expected_size) {
	type = DDSN_PEER_MESSAGE_TYPE_STRING;
}

void peer_prove_identity::feed(const std::string &line, int &type, size_t &expected_size) {

}

void peer_prove_identity::feed(const char *data, size_t size, int &type, size_t &expected_size) {
}

void peer_prove_identity::send() {
	string sign_message = "Sign this random number: " + boost::lexical_cast<std::string>(foreign_peer_->verification_number());

	cout << "PEER#" << connection_->id() << " asked to sign '" << sign_message << "'" << endl;

	connection_->send("PROVE IDENTITY\n" +
		sign_message + "\n");
}

// VERIFY IDENTITY

peer_verify_identity::peer_verify_identity(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection::pointer connection) :
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

void peer_verify_identity::send() {
}
