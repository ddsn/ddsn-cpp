#include "peer_messages.h"

#include "api_server.h"
#include "definitions.h"

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

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
	} else if (first_line == "INTRODUCE PEER") {
		return new peer_introduce(local_peer, connection);
	} else if (first_line == "CONNECT IN") {
		return new peer_connect(local_peer, connection);
	} else if (first_line == "STORE BLOCK") {
		return new peer_store_block(local_peer, connection);
	} else if (first_line == "LOAD BLOCK") {
		return new peer_load_block(local_peer, connection);
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

void peer_message::send(const char *bytes, size_t size) {
	connection_->send(bytes, size);
}

// HELLO

peer_hello::peer_hello(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection), state_(0) {

}

peer_hello::peer_hello(local_peer &local_peer, peer_connection::pointer connection, string type) :
peer_message(local_peer, connection), type_(type), state_(0) {

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

			connection_->foreign_peer()->set_public_key_str(public_key_);
			connection_->foreign_peer()->set_verification_number(rand()); // TODO: make it random

			cout << "PEER#" << connection_->id() << " random number: " << connection_->foreign_peer()->verification_number() << endl;

			state_ = 2;

			type = DDSN_MESSAGE_TYPE_STRING;
		} else {
			public_key_ += line + "\n";

			type = DDSN_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 2) {
		if (line == "") {
			if (connection_->foreign_peer()->host() == "" || connection_->foreign_peer()->port() == -1) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			} else {
				peer_prove_identity(local_peer_, connection_).send();

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
					connection_->foreign_peer()->set_queued(false);
				}
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

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_hello::send() {
	peer_message::send("HELLO\n");
	peer_message::send((char *)local_peer_.id().id(), 32);

	// send public key in pem format

	BIO *pub = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPublicKey(pub, local_peer_.keypair());

	size_t pub_len = BIO_pending(pub);

	char *pub_key = new char[pub_len];

	BIO_read(pub, pub_key, pub_len);

	// actually a string, but we can use byte verion of send
	peer_message::send(pub_key, pub_len);

	peer_message::send("Host: " + local_peer_.host() + "\n"
		"Port: " + boost::lexical_cast<string>(local_peer_.port()) + "\n"
		"Type: " + type_ + "\n"
		"\n");
}

// PROVE IDENTITY

peer_prove_identity::peer_prove_identity(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_prove_identity::~peer_prove_identity() {

}

void peer_prove_identity::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_prove_identity::feed(const std::string &line, int &type, size_t &expected_size) {
	if (line == "") {
		peer_verify_identity message(local_peer_, connection_);
		message.set_message(message_);
		message.send();

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		message_ += line;

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_prove_identity::feed(const char *data, size_t size, int &type, size_t &expected_size) {
}

void peer_prove_identity::send() {
	string sign_message = "Sign this random number: " + boost::lexical_cast<std::string>(connection_->foreign_peer()->verification_number());

	peer_message::send("PROVE IDENTITY\n" +
		sign_message + "\n\n");
}

// VERIFY IDENTITY

peer_verify_identity::peer_verify_identity(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_verify_identity::~peer_verify_identity() {

}

void peer_verify_identity::set_message(const std::string &message) {
	message_ = message;
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
		string sign_message = "Sign this random number: " + boost::lexical_cast<std::string>(connection_->foreign_peer()->verification_number());

		int ret = RSA_verify(NID_sha1, (unsigned char *)sign_message.c_str(), sign_message.length(),
			(unsigned char *)data, signature_length_, connection_->foreign_peer()->public_key());

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
}

void peer_verify_identity::send() {
	unsigned char *sigret = new unsigned char[RSA_size(local_peer_.keypair())];
	unsigned int siglen;

	RSA_sign(NID_sha1, (unsigned char *)message_.c_str(), message_.length(), sigret, &siglen, local_peer_.keypair());

	peer_message::send("VERIFY IDENTITY\n"
		"Signature-length: " + boost::lexical_cast<string>(siglen) + "\n\n");
	peer_message::send((char *)sigret, siglen);

	delete[] sigret;
}

// WELCOME

peer_welcome::peer_welcome(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_welcome::~peer_welcome() {

}

void peer_welcome::first_action(int &type, size_t &expected_size) {
	connection_->set_got_welcome(true);

	if (connection_->foreign_peer() && connection_->foreign_peer()->identity_verified()) {
		local_peer_.add_foreign_peer(connection_->foreign_peer());
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

void peer_set_code::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_set_code::feed(const std::string &line, int &type, size_t &expected_size) {
	if (line == "") {
		local_peer_.set_code(code_);
		peer_get_code(local_peer_, connection_, code_).send();

		int layer = code_.layers() - 1;

		connection_->foreign_peer()->set_out_layer(layer);
		connection_->foreign_peer()->set_in_layer(layer);

		local_peer_.set_integrated(true);

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

void peer_set_code::feed(const char *data, size_t size, int &type, size_t &expected_size) {

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

void peer_get_code::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_get_code::feed(const std::string &line, int &type, size_t &expected_size) {
	if (line == "") {
		// first, check if the code is correct

		if (local_peer_.code().layers() + 1 != code_.layers()) {
			return;
		}

		for (int i = 0; i < local_peer_.code().layers(); i++) {
			if (code_.layer_code(i) != code_.layer_code(i)) {
				return;
			}
		}

		// code is correct!

		int layer = local_peer_.code().layers();

		connection_->foreign_peer()->set_out_layer(layer);
		connection_->foreign_peer()->set_in_layer(layer);

		code local_code = local_peer_.code();
		local_code.resize_layers(layer + 1);
		local_code.set_layer_code(layer, 0);
		local_peer_.set_code(local_code);

		// TODO: Introcude peers

		cout << "Should introduce peers now..." << endl;

		local_peer_.set_splitting(false);

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

void peer_get_code::feed(const char *data, size_t size, int &type, size_t &expected_size) {

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

void peer_introduce::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_introduce::feed(const std::string &line, int &type, size_t &expected_size) {
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

void peer_introduce::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	peer_id peer_id;
	peer_id.set_id((unsigned char *)data);

	auto it = local_peer_.foreign_peers().find(peer_id);
	if (it != local_peer_.foreign_peers().end()) {
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
		it->second->set_out_layer(layer_);
	} else {
		shared_ptr<foreign_peer> new_peer(new foreign_peer());
		local_peer_.connect(host_, port_, new_peer, "introduce");
		new_peer->set_out_layer(layer_);
	}

	type = DDSN_MESSAGE_TYPE_END;
}

void peer_introduce::send() {
	peer_message::send("INTRODUCE PEER\n"
		"Host: " + host_ + "\n"
		"Port: " + boost::lexical_cast<string>(port_) + "\n"
		"Layer: " + boost::lexical_cast<string>(layer_)+"\n"
		"\n");
	peer_message::send((char *)peer_id_.id(), 32);
}

// CONNECT IN

peer_connect::peer_connect(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_connect::peer_connect(local_peer &local_peer, peer_connection::pointer connection, int layer, code code) :
peer_message(local_peer, connection) {

}

peer_connect::~peer_connect() {

}

void peer_connect::first_action(int &type, size_t &expected_size) {

}

void peer_connect::feed(const std::string &line, int &type, size_t &expected_size) {
}

void peer_connect::feed(const char *data, size_t size, int &type, size_t &expected_size) {

}

void peer_connect::send() {
	peer_message::send("CONNECT IN\n"
		"Code: " + code_.string() + "\n"
		"Layer: " + boost::lexical_cast<string>(layer_) + "\n"
		"\n");
}

// STORE BLOCK

peer_store_block::peer_store_block(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_store_block::peer_store_block(local_peer &local_peer, peer_connection::pointer connection, const block *block) :
peer_message(local_peer, connection), block_(block) {

}

peer_store_block::~peer_store_block() {

}

void peer_store_block::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_store_block::feed(const std::string &line, int &type, size_t &expected_size) {
	if (line == "") {
		type = DDSN_MESSAGE_TYPE_BYTES;
		expected_size = size_;
	}
	else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Code") {
			code_ = code(field_value, '_');
		} else if (field_name == "Name") {
			name_ = field_value;
		} else if (field_name == "Size") {
			try {
				size_ = stoi(field_value);
			} catch (...) {

			}
		}

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_store_block::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	block block(name_);

	block.set_data(data, size_);

	if (block.code() != code_) {
		cout << "Block code is wrong" << endl;
		return;
	}

	local_peer_.store(block);

	type = DDSN_MESSAGE_TYPE_END;
}

void peer_store_block::send() {
	peer_message::send("STORE BLOCK\n"
		"Code: " + block_->code().string('_') + "\n"
		"Name: " + block_->name() + "\n"
		"Size: " + boost::lexical_cast<string>(block_->size()) + "\n"
		"\n");
	peer_message::send(block_->data(), block_->size());
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

void peer_load_block::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void action_peer_load_block(local_peer &local_peer, peer_connection::pointer connection, block &block) {
	peer_deliver_block(local_peer, connection, &block).send();
}

void peer_load_block::feed(const std::string &line, int &type, size_t &expected_size) {
	if (line == "") {
		local_peer_.load(code_, boost::bind(&action_peer_load_block, local_peer_, connection_, _1));

		type = DDSN_MESSAGE_TYPE_END;
	}
	else {
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

void peer_load_block::feed(const char *data, size_t size, int &type, size_t &expected_size) {

}

void peer_load_block::send() {
	peer_message::send("LOAD BLOCK\n"
		"Code: " + code_.string('_') + "\n"
		"\n");
}

// DELIVER BLOCK

peer_deliver_block::peer_deliver_block(local_peer &local_peer, peer_connection::pointer connection) :
peer_message(local_peer, connection) {

}

peer_deliver_block::peer_deliver_block(local_peer &local_peer, peer_connection::pointer connection, const block *block) :
peer_message(local_peer, connection), block_(block) {

}

peer_deliver_block::~peer_deliver_block() {

}

void peer_deliver_block::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void peer_deliver_block::feed(const std::string &line, int &type, size_t &expected_size) {
	if (line == "") {
		if (size_ == 0) {
			block block(code_);
			local_peer_.do_load_actions(block);

			type = DDSN_MESSAGE_TYPE_END;
		}
		else {
			expected_size = size_;
			type = DDSN_MESSAGE_TYPE_BYTES;
		}
	}
	else {
		size_t colon_pos = line.find(": ");
		if (colon_pos == string::npos) {
			type = DDSN_MESSAGE_TYPE_ERROR;
			return;
		}
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "Code") {
			code_ = code(field_value, '_');
		} else if (field_name == "Name") {
			name_ = field_value;
		} else if (field_name == "Size") {
			try {
				size_ = stoi(field_value);
			} catch (...) {

			}
		}

		type = DDSN_MESSAGE_TYPE_STRING;
	}
}

void peer_deliver_block::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	block block(name_);

	block.set_data(data, size_);

	if (block.code() != code_) {
		cout << "Wrong block code" << endl;
	} else {
		local_peer_.do_load_actions(block);
	}
}

void peer_deliver_block::send() {
	if (block_->size() == 0) {
		peer_message::send("DELIVER BLOCK\n"
			"Code: " + block_->code().string() + "\n"
			"Size: 0\n"
			"\n");
	} else {
		peer_message::send("DELIVER BLOCK\n"
			"Code: " + block_->code().string() + "\n"
			"Name: " + block_->name() + "\n"
			"Size: " + boost::lexical_cast<string>(block_->size()) + "\n"
			"\n");
		peer_message::send(block_->data(), block_->size());
	}
}
