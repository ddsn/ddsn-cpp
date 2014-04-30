#include "peer_messages.h"

#include "definitions.h"

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <boost/lexical_cast.hpp>

using namespace ddsn;
using namespace std;

peer_message *peer_message::create_message(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection, const string &first_line) {
	if (first_line == "HELLO") {
		return new peer_hello(local_peer, foreign_peer, connection);
	} else if (first_line == "PROVE IDENTITY") {
		return new peer_prove_identity(local_peer, foreign_peer, connection);
	} else if (first_line == "VERIFY IDENTITY") {
		return new peer_verify_identity(local_peer, foreign_peer, connection);
	} else if (first_line == "WELCOME") {
		return new peer_welcome(local_peer, foreign_peer, connection);
	}
	return nullptr;
}

peer_message::peer_message(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection) :
	local_peer_(local_peer), foreign_peer_(foreign_peer), connection_(connection) {

}

peer_message::~peer_message() {

}

// HELLO

peer_hello::peer_hello(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection) :
	peer_message(local_peer, foreign_peer, connection), state_(0) {

}

peer_hello::~peer_hello() {

}

void peer_hello::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_BYTES;
	expected_size = 32;
}

void peer_hello::feed(const std::string &line, int &type, size_t &expected_size) {
	if (state_ == 0) {
		if (line != "-----BEGIN RSA PUBLIC KEY-----") {
			public_key_ += line + "\n";

			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		} else {
			public_key_ = "-----BEGIN RSA PUBLIC KEY-----\n";

			state_ = 1;
			type = DDSN_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 1) {
		if (line == "-----END RSA PUBLIC KEY-----") {
			public_key_ += "-----END RSA PUBLIC KEY-----\n";

			foreign_peer_->set_public_key_str(public_key_);
			foreign_peer_->set_verification_number(rand()); // TODO: make it random

			cout << "PEER#" << connection_->id() << " random number: " << foreign_peer_->verification_number() << endl;

			state_ = 2;

			type = DDSN_MESSAGE_TYPE_STRING;
		} else {
			public_key_ += line + "\n";

			type = DDSN_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 2) {
		if (line == "") {
			if (foreign_peer_->host() == "" || foreign_peer_->port() == -1) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			} else {
				peer_prove_identity(local_peer_, foreign_peer_, connection_).send();

				type = DDSN_MESSAGE_TYPE_END;
			}
		} else {
			size_t colon_pos = line.find(": ");
			if (colon_pos == string::npos) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
			string field_name = line.substr(0, colon_pos);
			string field_value = line.substr(colon_pos + 2);

			if (field_name == "Host") {
				foreign_peer_->set_host(field_value);
			} else if (field_name == "Port") {
				foreign_peer_->set_port(stoi(field_value));
			}
		}
	}
}

void peer_hello::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	if (size == 32) {
		peer_id foreign_id((unsigned char *)data);

		if (foreign_id == local_peer_.id()) {
			// that's myself!
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}

		foreign_peer_ = std::shared_ptr<foreign_peer>(new foreign_peer());
		foreign_peer_->set_id(foreign_id);

		connection_->set_foreign_peer(foreign_peer_);

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_hello::send() {
	connection_->send("HELLO\n");
	connection_->send((char *)local_peer_.id().id(), 32);

	// send public key in pem format

	BIO *pub = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPublicKey(pub, local_peer_.keypair());

	size_t pub_len = BIO_pending(pub);

	char *pub_key = new char[pub_len];

	BIO_read(pub, pub_key, pub_len);

	// actually a string, but we can use byte verion of send
	connection_->send(pub_key, pub_len);

	connection_->send("Host: " + local_peer_.host() + "\n"
		"Port: " + boost::lexical_cast<string>(local_peer_.id()) + "\n\n");
}

// PROVE IDENTITY

peer_prove_identity::peer_prove_identity(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection) :
	peer_message(local_peer, foreign_peer, connection) {

}

peer_prove_identity::~peer_prove_identity() {

}

void peer_prove_identity::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_prove_identity::feed(const std::string &line, int &type, size_t &expected_size) {
	if (line == "") {
		peer_verify_identity(local_peer_, foreign_peer_, connection_).send(message_);

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		message_ += line;

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_prove_identity::feed(const char *data, size_t size, int &type, size_t &expected_size) {
}

void peer_prove_identity::send() {
	string sign_message = "Sign this random number: " + boost::lexical_cast<std::string>(foreign_peer_->verification_number());

	connection_->send("PROVE IDENTITY\n" +
		sign_message + "\n\n");
}

// VERIFY IDENTITY

peer_verify_identity::peer_verify_identity(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection) :
	peer_message(local_peer, foreign_peer, connection) {

}

peer_verify_identity::~peer_verify_identity() {

}

void peer_verify_identity::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_verify_identity::feed(const std::string &line, int &type, size_t &expected_size) {
	if (line == "") {
		type = DDSN_MESSAGE_TYPE_BYTES;
		expected_size = signature_length_;
	} else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Signature-length") {
			signature_length_ = stoi(field_value);
		}
	}
}

void peer_verify_identity::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	if (size == signature_length_) {
		string sign_message = "Sign this random number: " + boost::lexical_cast<std::string>(foreign_peer_->verification_number());

		int ret = RSA_verify(NID_sha1, (unsigned char *)sign_message.c_str(), sign_message.length(),
			(unsigned char *)data, signature_length_, foreign_peer_->public_key());

		if (ret == 1) { // successful
			foreign_peer_->set_identity_verified(true);

			cout << "PEER#" << connection_->id() << " peer " << foreign_peer_->id().short_string() << " is now verified" << endl;

			if (connection_->got_welcome()) {
				local_peer_.add_foreign_peer(foreign_peer_);
			}

			peer_welcome(local_peer_, foreign_peer_, connection_).send();

			if (!connection_->introduced()) {
				peer_hello(local_peer_, foreign_peer_, connection_).send();
				connection_->set_introduced(true);
			}

			type = DDSN_MESSAGE_TYPE_END;
		} else {
			cout << "PEER#" << connection_->id() << " peer claiming to be " << foreign_peer_->id().short_string() << " gave an invalid signature" << endl;
			type = DDSN_MESSAGE_TYPE_ERROR;
		}
	}
}

void peer_verify_identity::send(string message) {
	unsigned char *sigret = new unsigned char[RSA_size(local_peer_.keypair())];
	unsigned int siglen;

	RSA_sign(NID_sha1, (unsigned char *)message.c_str(), message.length(), sigret, &siglen, local_peer_.keypair());

	connection_->send("VERIFY IDENTITY\n"
		"Signature-length: " + boost::lexical_cast<string>(siglen) + "\n\n");
	connection_->send((char *)sigret, siglen);

	delete[] sigret;
}

// WELCOME

peer_welcome::peer_welcome(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection) :
	peer_message(local_peer, foreign_peer, connection) {

}

peer_welcome::~peer_welcome() {

}

void peer_welcome::first_action(int &type, size_t &expected_size) {
	connection_->set_got_welcome(true);

	if (foreign_peer_ && foreign_peer_->identity_verified()) {
		local_peer_.add_foreign_peer(foreign_peer_);
	}

	type = DDSN_MESSAGE_TYPE_END;
}

void peer_welcome::feed(const std::string &line, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}

void peer_welcome::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}

void peer_welcome::send() {
	connection_->send("WELCOME\n");
}