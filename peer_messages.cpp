#include "peer_messages.h"

#include "api_server.h"
#include "definitions.h"
#include "utilities.h"

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <cstring>

using namespace ddsn;
using namespace std;

peer_message *peer_message::create_message(local_peer &local_peer, peer_connection::pointer connection, const string &first_line) {
	if (first_line == "HELLO") {
		return new peer_hello(local_peer, connection);
	} else if (first_line == "PROVE IDENTITY") {
		return new peer_prove_identity(local_peer, connection);
	} else if (first_line == "VERIFY IDENTITY") {
		return new peer_verify_identity(local_peer, connection);
	} else if (first_line == "WELCOME") {
		return new peer_welcome(local_peer, connection);
	} else if (first_line == "SET CODE") {
		return new peer_set_code(local_peer, connection);
	} else if (first_line == "GET CODE") {
		return new peer_get_code(local_peer, connection);
	} else if (first_line == "INTEGRATED") {
		return new peer_integrated(local_peer, connection);
	} else if (first_line == "INTRODUCE PEER") {
		return new peer_introduce(local_peer, connection);
	} else if (first_line == "CONNECT IN") {
		return new peer_connect(local_peer, connection);
	} else if (first_line == "STORE BLOCK") {
		return new peer_store_block(local_peer, connection);
	} else if (first_line == "LOAD BLOCK") {
		return new peer_load_block(local_peer, connection);
	} else if (first_line == "STORED BLOCK") {
		return new peer_stored_block(local_peer, connection);
	} else if (first_line == "DELIVER BLOCK") {
		return new peer_deliver_block(local_peer, connection);
	}
	return nullptr;
}

peer_message::peer_message(local_peer &local_peer, peer_connection::pointer connection) :
local_peer_(local_peer), connection_(connection) {

}

peer_message::~peer_message() {

}

void peer_message::send(const std::string &string) {
	connection_->send(string);
}

void peer_message::send(const BYTE *bytes, size_t size) {
	connection_->send(bytes, size);
}

// HELLO

/* 
 * First message to be sent to a newly connected peer.
 * Contains the public key and host and port which will be needed when introducing 
 * this peer to other peers later.
 * The type denotes for which reason the peer connected. Currently, this can
 * be either the intention to become part of the network or the intention of
 * getting a new out connection.
 * == Sending perspective ==
 * Say hello to a new peer. Either queue up for getting part of the network (type == 'queued')
 * or otherwise tell your intention later (type != 'queued')
 * == Receiving perspective ==
 * As a response the connecting peer is requested to prove its identity by
 * delivering a message which has to be signed by the connecting peer. (see @peer_prove_identity)
 */

peer_hello::peer_hello(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection), state_(0) {

}

peer_hello::peer_hello(local_peer &local_peer, peer_connection::pointer connection, string type) :
peer_message(local_peer, connection), type_(type), state_(0) {

}

peer_hello::~peer_hello() {

}

void peer_hello::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_hello::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (state_ == 0) {
		if (line == "") {
			state_ = 1;
		} else {
			size_t colon_pos = line.find(": ");
			if (colon_pos == string::npos) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
			string field_name = line.substr(0, colon_pos);
			string field_value = line.substr(colon_pos + 2);

			if (field_name == "Id") {
				if (field_value.length() == 64) {
					BYTE foreign_id_bytes[32];
					hex_to_bytes(field_value, foreign_id_bytes, 32);
					peer_id foreign_id(foreign_id_bytes);

					if (foreign_id == local_peer_.id()) {
						// that's myself!
						type = DDSN_MESSAGE_TYPE_ERROR;
						return;
					}

					auto it = local_peer_.foreign_peers().find(foreign_id);
					if (it != local_peer_.foreign_peers().end()) {
						// already know you!
						if (it->second->connected()) {
							// and I'm already connected to you!
							type = DDSN_MESSAGE_TYPE_ERROR;
							return;
						} else {
							connection_->set_foreign_peer(it->second);
						}
					} else {
						connection_->foreign_peer()->set_id(foreign_id);
					}
				} else {
					type = DDSN_MESSAGE_TYPE_END;
				}
			} else if (field_name == "Host") {
				if (field_value == "") {
					connection_->foreign_peer()->set_host(boost::lexical_cast<std::string>(connection_->socket().remote_endpoint().address()));
				} else {
					connection_->foreign_peer()->set_host(field_value);
				}
			} else if (field_name == "Port") {
				try {
					connection_->foreign_peer()->set_port(stoi(field_value));
				} catch (...) {
					type = DDSN_MESSAGE_TYPE_ERROR;
					return;
				}
			} else if (field_name == "Type") {
				if (field_value == "queued") {
					connection_->foreign_peer()->set_queued(true);
				} else {
					// for example: introduce
					connection_->foreign_peer()->set_queued(false);
				}
			}
		}
	} else if (state_ == 1) {
		if (line != "-----BEGIN RSA PUBLIC KEY-----") {
			public_key_ += line + "\n";

			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		} else {
			public_key_ = "-----BEGIN RSA PUBLIC KEY-----\n";

			state_ = 2;
			type = DDSN_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 2) {
		if (line == "-----END RSA PUBLIC KEY-----") {
			public_key_ += "-----END RSA PUBLIC KEY-----\n";

			connection_->foreign_peer()->set_public_key_str(public_key_);

			// check if this matches the id

			BYTE hash[32];
			hash_from_rsa(connection_->foreign_peer()->public_key(), hash);

			if (memcmp(hash, connection_->foreign_peer()->id().id(), 32) != 0) {
				cout << "Peer appears to be a fraud" << endl;
				type = DDSN_MESSAGE_TYPE_ERROR;
			}

			connection_->foreign_peer()->set_verification_number(rand()); // TODO: make it random

			cout << "PEER#" << connection_->id() << " random number: " << connection_->foreign_peer()->verification_number() << endl;

			if (connection_->foreign_peer()->host() == "" || connection_->foreign_peer()->port() == -1) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			} else {
				peer_prove_identity(local_peer_, connection_).send();

				type = DDSN_MESSAGE_TYPE_END;
			}

			type = DDSN_MESSAGE_TYPE_END;
		} else {
			public_key_ += line + "\n";

			type = DDSN_MESSAGE_TYPE_STRING;
		}
	}
}

void peer_hello::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {

}

void peer_hello::send() {
	peer_message::send("HELLO\n"
		"Id: " + bytes_to_hex(local_peer_.id().id(), 32) + "\n"
		"Host: " + local_peer_.host() + "\n"
		"Port: " + boost::lexical_cast<string>(local_peer_.port()) + "\n"
		"Type: " + type_ + "\n"
		"\n");

	// send public key in pem format

	BIO *pub = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPublicKey(pub, local_peer_.keypair());

	size_t pub_len = BIO_pending(pub);

	BYTE *pub_key = new BYTE[pub_len];

	BIO_read(pub, pub_key, pub_len);

	BIO_free(pub);

	// actually a string, but we can use byte verion of send
	peer_message::send(pub_key, pub_len);

	delete[] pub_key;
}

// PROVE IDENTITY

/*
 * == Sending perspecive ==
 * Sends a message to be signed by the connected peer in order to prove its identity.
 * == Receiving perspective ==
 * Reads a message, signs it and sends the signature back. (see @peer_verify_identity)
 */

peer_prove_identity::peer_prove_identity(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_prove_identity::~peer_prove_identity() {

}

void peer_prove_identity::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_prove_identity::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (line == "") {
		peer_verify_identity message(local_peer_, connection_, message_);
		message.send();

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		message_ += line;

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_prove_identity::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {
}

void peer_prove_identity::send() {
	string sign_message = "Sign this random number: " + boost::lexical_cast<std::string>(connection_->foreign_peer()->verification_number());

	peer_message::send("PROVE IDENTITY\n" +
		sign_message + "\n\n");
}

// VERIFY IDENTITY

/*
 * 
 */

peer_verify_identity::peer_verify_identity(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_verify_identity::peer_verify_identity(local_peer &local_peer, peer_connection::pointer connection, const std::string &message) :
peer_message(local_peer, connection), message_(message) {

}

peer_verify_identity::~peer_verify_identity() {

}

void peer_verify_identity::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_BYTES;
	expected_size = 256;
}

void peer_verify_identity::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
}

void peer_verify_identity::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {
	string sign_message = "Sign this random number: " + boost::lexical_cast<std::string>(connection_->foreign_peer()->verification_number());
	
	BYTE message_hash[32];

	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, sign_message.c_str(), sign_message.length());
	SHA256_Final(message_hash, &sha256);

	int ret = RSA_verify(NID_sha256, message_hash, 32,
		(BYTE *)data, 256, connection_->foreign_peer()->public_key());

	if (ret == 1) { // successful
		connection_->foreign_peer()->set_identity_verified(true);

		cout << "PEER#" << connection_->id() << " peer " << connection_->foreign_peer()->id().short_string() << " is now verified" << endl;

		if (connection_->got_welcome()) {
			local_peer_.add_foreign_peer(connection_->foreign_peer());
		}

		peer_welcome(local_peer_, connection_).send();

		if (!connection_->introduced()) {
			peer_hello(local_peer_, connection_, "?").send();
			connection_->set_introduced(true);
		}

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		cout << "PEER#" << connection_->id() << " peer claiming to be " << connection_->foreign_peer()->id().short_string() << " gave an invalid signature" << endl;
		type = DDSN_MESSAGE_TYPE_ERROR;
	}
}

void peer_verify_identity::send() {
	BYTE *sigret = new BYTE[RSA_size(local_peer_.keypair())];
	UINT32 siglen;

	BYTE message_hash[32];

	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, message_.c_str(), message_.length());
	SHA256_Final(message_hash, &sha256);

	RSA_sign(NID_sha256, message_hash, 32, sigret, &siglen, local_peer_.keypair());

	peer_message::send("VERIFY IDENTITY\n");
	peer_message::send(sigret, 256);

	delete[] sigret;
}

// WELCOME

peer_welcome::peer_welcome(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_welcome::~peer_welcome() {

}

void peer_welcome::first_action(UINT32 &type, size_t &expected_size) {
	connection_->set_got_welcome(true);

	if (connection_->foreign_peer() && connection_->foreign_peer()->identity_verified()) {
		local_peer_.add_foreign_peer(connection_->foreign_peer());
	}

	type = DDSN_MESSAGE_TYPE_END;
}

void peer_welcome::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}

void peer_welcome::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}

void peer_welcome::send() {
	peer_message::send("WELCOME\n");
}

// SET CODE

peer_set_code::peer_set_code(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_set_code::peer_set_code(local_peer &local_peer, peer_connection::pointer connection, code code) :
peer_message(local_peer, connection), code_(code) {

}

peer_set_code::~peer_set_code() {

}

void peer_set_code::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_set_code::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (line == "") {
		local_peer_.set_code(code_);
		peer_get_code(local_peer_, connection_, code_).send();

		int layer = code_.layers() - 1;

		connection_->foreign_peer()->set_out_layer(layer);
		connection_->foreign_peer()->set_in_layer(layer);

		local_peer_.set_mentor(connection_->foreign_peer());

		if (layer == 0) {
			// got all needed peers
			local_peer_.set_integrated(true);
			peer_integrated(local_peer_, connection_).send();
		} else {
			// wait for peers to be introduced
		}

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Code") {
			code_ = code(field_value, '_');
		}

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_set_code::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {

}

void peer_set_code::send() {
	peer_message::send("SET CODE\n"
		"Code: " + code_.string('_') + "\n"
		"\n");
}

// GET CODE

peer_get_code::peer_get_code(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_get_code::peer_get_code(local_peer &local_peer, peer_connection::pointer connection, code code) :
peer_message(local_peer, connection), code_(code) {

}

peer_get_code::~peer_get_code() {

}

void peer_get_code::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_get_code::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (line == "") {
		// first, check if the code is correct

		if (local_peer_.code().layers() + 1 != code_.layers()) {
			return;
		}

		for (UINT32 i = 0; i < local_peer_.code().layers(); i++) {
			if (code_.layer_code(i) != code_.layer_code(i)) {
				return;
			}
		}

		// code is correct!

		UINT32 layer = local_peer_.code().layers();

		connection_->foreign_peer()->set_out_layer(layer);
		connection_->foreign_peer()->set_in_layer(layer);

		code local_code = local_peer_.code();
		local_code.resize_layers(layer + 1);
		local_code.set_layer_code(layer, 0);
		local_peer_.set_code(local_code);

		cout << "Introducing peers..." << endl;

		for (UINT32 out_layer = 0; out_layer < layer; out_layer++) {
			auto out_peer = local_peer_.out_peer(out_layer);

			if (out_peer) {
				peer_introduce(local_peer_, connection_, out_peer->id(), out_peer->host(), out_peer->port(), out_layer).send();
			}
		}

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Code") {
			code_ = ddsn::code(field_value, '_');
		}

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_get_code::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {

}

void peer_get_code::send() {
	peer_message::send("GET CODE\n"
		"Code: " + code_.string('_') + "\n"
		"\n");
}

// INTRODUCE PEER

peer_introduce::peer_introduce(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_introduce::peer_introduce(local_peer &local_peer, peer_connection::pointer connection, peer_id peer_id, std::string host, int port, int layer) :
peer_message(local_peer, connection), peer_id_(peer_id), host_(host), port_(port), layer_(layer), state_(0) {

}

peer_introduce::~peer_introduce() {

}

void peer_introduce::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_introduce::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (line == "") {
		type = DDSN_MESSAGE_TYPE_BYTES;
		expected_size = 32;
	} else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Host") {
			host_ = field_value;
		} else if (field_name == "Port") {
			try {
				port_ = stoi(field_value);
			} catch (...) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
		} else if (field_name == "Layer") {
			try {
				layer_ = stoi(field_value);
			} catch (...) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
		}

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_introduce::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {
	peer_id peer_id;
	peer_id.set_id((BYTE *)data);

	auto it = local_peer_.foreign_peers().find(peer_id);
	if (it != local_peer_.foreign_peers().end()) {
		it->second->set_out_layer(layer_);
		if (it->second->connected()) {
			code code;
			code.resize_layers(layer_);
			for (int i = 0; i < layer_; i++) {
				code.set_layer_code(i, local_peer_.code().layer_code(i)); // TODO: implement a faster function for that in ddsn::code
			}
			peer_connect(local_peer_, it->second->connection(), layer_, code).send();
		} else {
			local_peer_.connect(host_, port_, it->second, "introduce");
		}
	} else {
		shared_ptr<foreign_peer> new_peer(new foreign_peer());
		new_peer->set_out_layer(layer_);
		local_peer_.connect(host_, port_, new_peer, "introduce");
	}

	type = DDSN_MESSAGE_TYPE_END;
}

void peer_introduce::send() {
	peer_message::send("INTRODUCE PEER\n"
		"Host: " + host_ + "\n"
		"Port: " + boost::lexical_cast<string>(port_) + "\n"
		"Layer: " + boost::lexical_cast<string>(layer_)+"\n"
		"\n");
	peer_message::send(peer_id_.id(), 32);
}

// CONNECT IN

peer_connect::peer_connect(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_connect::peer_connect(local_peer &local_peer, peer_connection::pointer connection, int layer, code code) :
peer_message(local_peer, connection), layer_(layer), code_(code) {

}

peer_connect::~peer_connect() {

}

void peer_connect::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_connect::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (line == "") {
		// TODO: Check code and layer
		connection_->foreign_peer()->set_in_layer(layer_);

		type = DDSN_MESSAGE_TYPE_END;
		return;
	} else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Code") {
			code_ = code(field_value);
		} else if (field_name == "Layer") {
			try {
				layer_ = stoi(field_value);
			} catch (...) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
		}

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_connect::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {

}

void peer_connect::send() {
	peer_message::send("CONNECT IN\n"
		"Code: " + code_.string() + "\n"
		"Layer: " + boost::lexical_cast<string>(layer_) + "\n"
		"\n");
}

// INTEGRATED

peer_integrated::peer_integrated(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_integrated::~peer_integrated() {

}

void peer_integrated::first_action(UINT32 &type, size_t &expected_size) {
	connection_->foreign_peer()->set_integrated(true);

	// hand block
	local_peer_.redistribute_block();

	type = DDSN_MESSAGE_TYPE_END;
}

void peer_integrated::feed(const std::string &line, UINT32 &type, size_t &expected_size) {

}

void peer_integrated::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {

}

void peer_integrated::send() {
	peer_message::send("INTEGRATED\n");
}

// STORE BLOCK

peer_store_block::peer_store_block(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection), state_(0) {

}

peer_store_block::peer_store_block(local_peer &local_peer, peer_connection::pointer connection, const block &block) :
peer_message(local_peer, connection), block_(block) {

}

peer_store_block::~peer_store_block() {

}

void peer_store_block::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_store_block::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (state_ == 0) {
		if (line == "") {
			type = DDSN_MESSAGE_TYPE_BYTES;
			expected_size = 256;
		} else {
			size_t colon_pos = line.find(": ");
			if (colon_pos == string::npos) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
			string field_name = line.substr(0, colon_pos);
			string field_value = line.substr(colon_pos + 2);

			if (field_name == "Code") {
				block_.set_code(code(field_value, '_'));
			} else if (field_name == "Name") {
				block_.set_name(field_value);
			} else if (field_name == "Occurrence") {
				try {
					block_.set_occurrence(stoi(field_value));
				} catch (...) {
					type = DDSN_MESSAGE_TYPE_ERROR;
					return;
				}
			} else if (field_name == "Size") {
				try {
					block_.set_size(stoi(field_value));
				} catch (...) {
					type = DDSN_MESSAGE_TYPE_ERROR;
					return;
				}
			}

			type = DDSN_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 1) {
		// public key

		if (line == "") {
			type = DDSN_MESSAGE_TYPE_BYTES;
			expected_size = block_.size();
		} else {
			public_key_ += line + "\n";
			type = DDSN_MESSAGE_TYPE_STRING;
		}
	}
}

static void action_peer_store_block(local_peer &local_peer, peer_connection::pointer connection, const block &block, bool success) {
	peer_stored_block(local_peer, connection, block, success).send();
}

void peer_store_block::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {
	if (state_ == 0) {
		// got signature

		BYTE signature[256];
		memcpy(signature, data, 256);
		block_.set_signature(signature);

		state_ = 1;

		type = DDSN_MESSAGE_TYPE_STRING;
	} else if (state_ == 1) {
		// owner

		BIO *pub = BIO_new(BIO_s_mem());

		BIO_write(pub, public_key_.c_str(), public_key_.length());

		RSA *owner = nullptr;
		PEM_read_bio_RSAPublicKey(pub, &owner, NULL, NULL);

		BIO_free(pub);

		block_.set_owner(owner);

		// data

		block_.set_data(data, size);

		if (!block_.verify()) {
			cout << "Block is corrupted" << endl;
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}

		local_peer_.store(block_, boost::bind(&action_peer_store_block, boost::ref(local_peer_), connection_, _1, _2));

		type = DDSN_MESSAGE_TYPE_END;
	}
}

void peer_store_block::send() {
	peer_message::send("STORE BLOCK\n"
		"Code: " + block_.code().string('_') + "\n"
		"Name: " + block_.name() + "\n"
		"Occurrence: " + boost::lexical_cast<string>(block_.occurrence()) + "\n"
		"Size: " + boost::lexical_cast<string>(block_.size()) + "\n"
		"\n");

	// send signature

	peer_message::send(block_.signature(), 256);

	// send public key in pem format

	BIO *pub = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPublicKey(pub, block_.owner());

	size_t pub_len = BIO_pending(pub);

	BYTE *pub_key = new BYTE[pub_len];

	BIO_read(pub, pub_key, pub_len);

	BIO_free(pub);

	// actually a string, but we can use byte verion of send
	peer_message::send(pub_key, pub_len);
	peer_message::send("\n");

	delete[] pub_key;

	// send data
	peer_message::send(block_.data(), block_.size());
}

// LOAD BLOCK

peer_load_block::peer_load_block(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_load_block::peer_load_block(local_peer &local_peer, peer_connection::pointer connection, const code &code) :
peer_message(local_peer, connection), code_(code) {

}

peer_load_block::~peer_load_block() {

}

void peer_load_block::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void action_peer_load_block(local_peer &local_peer, peer_connection::pointer connection, const block &block, bool success) {
	peer_deliver_block(local_peer, connection, block, success).send();
}

void peer_load_block::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (line == "") {
		local_peer_.load(code_, boost::bind(&action_peer_load_block, boost::ref(local_peer_), connection_, _1, _2));

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Code") {
			code_ = code(field_value, '_');
		}

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_load_block::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {

}

void peer_load_block::send() {
	peer_message::send("LOAD BLOCK\n"
		"Code: " + code_.string('_') + "\n"
		"\n");
}

// STORED BLOCK

peer_stored_block::peer_stored_block(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_stored_block::peer_stored_block(local_peer &local_peer, peer_connection::pointer connection, const block &block, bool success) :
peer_message(local_peer, connection), block_(block::copy_without_data(block)), success_(success) {

}

peer_stored_block::~peer_stored_block() {

}

void peer_stored_block::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_stored_block::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (line == "") {
		local_peer_.do_store_actions(boost::ref(block_), success_);

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Code") {
			block_.set_code(code(field_value, '_'));
		} else if (field_name == "Name") {
			block_.set_name(field_value);
		} else if (field_name == "Occurrence") {
			try {
				block_.set_occurrence(stoi(field_value));
			} catch (...) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
		} else if (field_name == "Owner") {
			BYTE owner_hash[32];
			hex_to_bytes(field_value, owner_hash, 32);
			block_.set_owner_hash(owner_hash);
		} else if (field_name == "Success") {
			success_ = field_value == "yes";
		}

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_stored_block::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {

}

void peer_stored_block::send() {
	peer_message::send("STORED BLOCK\n"
		"Code: " + block_.code().string('_') + "\n"
		"Name: " + block_.name() + "\n"
		"Occurrence: " + boost::lexical_cast<string>(block_.occurrence()) + "\n"
		"Owner: " + bytes_to_hex(block_.owner_hash(), 32) + "\n"
		"Success: " + (success_ ? "yes" : "no") + "\n"
		"\n");
}

// DELIVER BLOCK

peer_deliver_block::peer_deliver_block(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection), state_(0) {

}

peer_deliver_block::peer_deliver_block(local_peer &local_peer, peer_connection::pointer connection, const block &block, bool success) :
peer_message(local_peer, connection), block_(block), success_(success) {

}

peer_deliver_block::~peer_deliver_block() {

}

void peer_deliver_block::first_action(UINT32 &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_deliver_block::feed(const std::string &line, UINT32 &type, size_t &expected_size) {
	if (state_ == 0) {
		if (line == "") {
			if (!success_) {
				local_peer_.do_load_actions(block_, false);

				type = DDSN_MESSAGE_TYPE_END;
			} else {
				expected_size = 256;
				type = DDSN_MESSAGE_TYPE_BYTES;
			}
		} else {
			size_t colon_pos = line.find(": ");
			if (colon_pos == string::npos) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
			string field_name = line.substr(0, colon_pos);
			string field_value = line.substr(colon_pos + 2);

			if (field_name == "Code") {
				block_.set_code(code(field_value, '_'));
			} else if (field_name == "Name") {
				block_.set_name(field_value);
			} else if (field_name == "Occurrence") {
				try {
					block_.set_occurrence(stoi(field_value));
				} catch (...) {
					type = DDSN_MESSAGE_TYPE_ERROR;
					return;
				}
			} else if (field_name == "Size") {
				try {
					block_.set_size(stoi(field_value));
				} catch (...) {
					type = DDSN_MESSAGE_TYPE_ERROR;
					return;
				}
			} else if (field_name == "Success") {
				success_ = field_value == "yes";
			}

			type = DDSN_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 1) {
		// public key

		if (line == "") {
			type = DDSN_MESSAGE_TYPE_BYTES;
			expected_size = block_.size();
		} else {
			public_key_ += line + "\n";
			type = DDSN_MESSAGE_TYPE_STRING;
		}
	}
}

void peer_deliver_block::feed(const BYTE *data, size_t size, UINT32 &type, size_t &expected_size) {
	if (state_ == 0) {
		// got signature

		BYTE signature[256];
		memcpy(signature, data, 256);
		block_.set_signature(signature);

		state_ = 1;

		type = DDSN_MESSAGE_TYPE_STRING;
	} else if (state_ == 1) {
		// owner

		BIO *pub = BIO_new(BIO_s_mem());

		BIO_write(pub, public_key_.c_str(), public_key_.length());

		RSA *owner = nullptr;
		PEM_read_bio_RSAPublicKey(pub, &owner, NULL, NULL);

		BIO_free(pub);

		block_.set_owner(owner);

		// data

		block_.set_data(data, size);

		if (!block_.verify()) {
			cout << "Block is corrupted" << endl;
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}

		local_peer_.do_load_actions(block_, true);

		type = DDSN_MESSAGE_TYPE_END;
	}
}

void peer_deliver_block::send() {
	if (!success_) {
		peer_message::send("DELIVER BLOCK\n"
			"Code: " + block_.code().string() + "\n"
			"Success: no\n"
			"\n");
	} else {
		peer_message::send("DELIVER BLOCK\n"
			"Code: " + block_.code().string('_') + "\n"
			"Name: " + block_.name() + "\n"
			"Occurrence: " + boost::lexical_cast<string>(block_.occurrence()) + "\n"
			"Size: " + boost::lexical_cast<string>(block_.size()) + "\n"
			"Success: yes\n"
			"\n");

		// send signature

		peer_message::send(block_.signature(), 256);

		// send public key in pem format

		BIO *pub = BIO_new(BIO_s_mem());

		PEM_write_bio_RSAPublicKey(pub, block_.owner());

		size_t pub_len = BIO_pending(pub);

		BYTE *pub_key = new BYTE[pub_len];

		BIO_read(pub, pub_key, pub_len);

		BIO_free(pub);

		// actually a string, but we can use byte verion of send
		peer_message::send(pub_key, pub_len);
		peer_message::send("\n");

		delete[] pub_key;

		// send data
		peer_message::send(block_.data(), block_.size());
	}
}
