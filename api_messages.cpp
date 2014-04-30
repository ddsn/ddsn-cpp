#include "api_messages.h"

#include "api_server.h"
#include "definitions.h"
#include "utilities.h"

#include <boost/lexical_cast.hpp>

using namespace ddsn;
using namespace std;

api_in_message *api_in_message::create_message(local_peer &local_peer, api_connection::pointer connection, const string &first_line) {
	if (first_line == "HELLO") {
		return new api_in_hello(local_peer, connection);
	}
	
	if (connection->authenticated()) {
		if (first_line == "PING") {
			return new api_in_ping(local_peer, connection);
		} else if (first_line == "STORE FILE") {
			return new api_in_store_file(local_peer, connection);
		} else if (first_line == "LOAD FILE") {
			return new api_in_load_file(local_peer, connection);
		} else if (first_line == "CONNECT PEER") {
			return new api_in_connect_peer(local_peer, connection);
		} else if (first_line == "PEER INFO") {
			return new api_in_peer_info(local_peer, connection);
		}
	} else {
		return nullptr;
	}

	return nullptr;
}

api_in_message::api_in_message(local_peer &local_peer, api_connection::pointer connection) : local_peer_(local_peer), connection_(connection) {

}

api_in_message::~api_in_message() {

}

api_out_message::api_out_message(const local_peer &local_peer) : local_peer_(local_peer) {

}

api_out_message::~api_out_message() {

}

void api_out_message::send(api_connection::pointer connection, const std::string &string) {
	connection->send(string);
}

void api_out_message::send(api_connection::pointer connection, const char *bytes, size_t size) {
	connection->send(bytes, size);
}

/*
  IN MESSAGES
*/

// HELLO

api_in_hello::api_in_hello(local_peer &local_peer, api_connection::pointer connection) : api_in_message(local_peer, connection) {

}

api_in_hello::~api_in_hello() {

}

void api_in_hello::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void api_in_hello::feed(const string &line, int &type, size_t &expected_size) {
	if (line == connection_->server().password()) {
		api_out_hello(local_peer_).send(connection_);

		connection_->set_authenticated(true);
		connection_->server().add_connection(connection_);

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		type = DDSN_MESSAGE_TYPE_ERROR;
	}
}

void api_in_hello::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}



// PING

api_in_ping::api_in_ping(local_peer &local_peer, api_connection::pointer connection) : api_in_message(local_peer, connection) {
	
}

api_in_ping::~api_in_ping() {

}

void api_in_ping::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_END;

	api_out_ping(local_peer_).send(connection_);
}

void api_in_ping::feed(const string &line, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}

void api_in_ping::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}

// STORE FILE

api_in_store_file::api_in_store_file(local_peer &local_peer, api_connection::pointer connection) :
api_in_message(local_peer, connection), state_(0), chunk_(0), data_pointer_(0), data_(nullptr) {
	
}

api_in_store_file::~api_in_store_file() {
	if (data_ != nullptr) {
		delete data_;
	}
}

void api_in_store_file::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void api_in_store_file::feed(const string &line, int &type, size_t &expected_size) {
	if (state_ == 0) { // File information
		if (line == "") { // End of file information
			state_ = 1;

			data_ = new char[file_size_];

			type = DDSN_MESSAGE_TYPE_STRING;
		} else {
			size_t colon_pos = line.find(": ");
			if (colon_pos == string::npos) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
			string field_name = line.substr(0, colon_pos);
			string field_value = line.substr(colon_pos + 2);

			if (field_name == "File-name") {
				file_name_ = field_value;
			} else if (field_name == "File-size") {
				file_size_ = stoi(field_value);

				if (file_size_ >= 8 * 1024 * 1024) {
					type = DDSN_MESSAGE_TYPE_ERROR;
					return;
				}
			} else if (field_name == "Chunks") {
				chunks_ = stoi(field_value);
			} else {
				// unknown field
			}

			type = DDSN_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 1) { // Chunk information
		if (line == "") {
			chunk_++;

			type = DDSN_MESSAGE_TYPE_BYTES;
			expected_size = chunk_size_;
		} else {
			size_t colon_pos = line.find(": ");
			if (colon_pos == string::npos) {
				type = DDSN_MESSAGE_TYPE_ERROR;
				return;
			}
			string field_name = line.substr(0, colon_pos);
			string field_value = line.substr(colon_pos + 2);

			if (field_name == "Chunk-size") {
				chunk_size_ = stoi(field_value);
			} else {
				// unknown field
			}

			type = DDSN_MESSAGE_TYPE_STRING;
		}
	}
}

void api_in_store_file::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;

	if (chunk_ < chunks_) {
		memcpy(data_ + data_pointer_, data, size);

		data_pointer_ += size;

		type = DDSN_MESSAGE_TYPE_STRING;
	} else {
		memcpy(data_ + data_pointer_, data, size);

		block block(file_name_);
		block.set_data(data_, file_size_);

		local_peer_.store(block);

		api_out_store_file(local_peer_, block.code()).send(connection_);

		type = DDSN_MESSAGE_TYPE_END;
	}
}

// LOAD FILE

api_in_load_file::api_in_load_file(local_peer &local_peer, api_connection::pointer connection) :
api_in_message(local_peer, connection) {

}

api_in_load_file::~api_in_load_file() {

}

void api_in_load_file::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void api_in_load_file::feed(const string &line, int &type, size_t &expected_size) {
	if (line == "") {
		block block(code_);
		local_peer_.load(block);

		if (block.size() > 0) {
			api_out_load_file(local_peer_, block.name(), block.size(), code_, block.data()).send(connection_);
		} else {
			api_out_load_file(local_peer_, code_).send(connection_);
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

		if (field_name == "Block-code") {
			code_ = code(field_value);
			type = DDSN_MESSAGE_TYPE_STRING;
		}
	}
}

void api_in_load_file::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}

// CONNECT PEER

api_in_connect_peer::api_in_connect_peer(local_peer &local_peer, api_connection::pointer connection) :
api_in_message(local_peer, connection) {

}

api_in_connect_peer::~api_in_connect_peer() {

}

void api_in_connect_peer::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;
}

void api_in_connect_peer::feed(const string &line, int &type, size_t &expected_size) {

	if (line == "") {
		local_peer_.connect(host_, port_);

		type = DDSN_MESSAGE_TYPE_END;
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
			port_ = stoi(field_value);
		}
	}
}

void api_in_connect_peer::feed(const char *data, size_t size, int &type, size_t &expected_size) {
}

// PEER INFO

api_in_peer_info::api_in_peer_info(local_peer &local_peer, api_connection::pointer connection) :
api_in_message(local_peer, connection) {

}

api_in_peer_info::~api_in_peer_info() {

}

void api_in_peer_info::first_action(int &type, size_t &expected_size) {
	api_out_peer_info(local_peer_).send(connection_);

	type = DDSN_MESSAGE_TYPE_END;
}

void api_in_peer_info::feed(const string &line, int &type, size_t &expected_size) {
}

void api_in_peer_info::feed(const char *data, size_t size, int &type, size_t &expected_size) {
}

/*
  OUT MESSAGES
*/

// HELLO

api_out_hello::api_out_hello(local_peer &local_peer) :
api_out_message(local_peer) {
}

api_out_hello::~api_out_hello() {
}

void api_out_hello::send(api_connection::pointer connection) {
	api_out_message::send(connection, "HELLO\n");
}

// PONG

api_out_ping::api_out_ping(local_peer &local_peer) :
api_out_message(local_peer) {
}

api_out_ping::~api_out_ping() {
}

void api_out_ping::send(api_connection::pointer connection) {
	api_out_message::send(connection, "PONG\n");
}

// STORE FILE

api_out_store_file::api_out_store_file(local_peer &local_peer, const code &block_code) :
api_out_message(local_peer), block_code_(block_code) {
}

api_out_store_file::~api_out_store_file() {
}

void api_out_store_file::send(api_connection::pointer connection) {
	api_out_message::send(connection, "STORE FILE\n"
		"Block-code: " + block_code_.string() + "\n"
		"\n");
}

// LOAD FILE

api_out_load_file::api_out_load_file(local_peer &local_peer, const code &block_code) :
api_out_message(local_peer), block_code_(block_code), data_(nullptr) {
}

api_out_load_file::api_out_load_file(local_peer &local_peer, const std::string &file_name, size_t file_size, const code &block_code, const char *data) :
api_out_message(local_peer), file_name_(file_name), file_size_(file_size), block_code_(block_code), data_(data) {
}

api_out_load_file::~api_out_load_file() {
}

void api_out_load_file::send(api_connection::pointer connection) {
	if (data_ != nullptr) {
		api_out_message::send(connection, "LOAD FILE\n"
			"Block-code: " + block_code_.string('_') + "\n"
			"File-name: " + file_name_ + "\n"
			"File-size: " + boost::lexical_cast<string>(file_size_)+"\n"
			"\n");
		api_out_message::send(connection, data_, file_size_);
	} else {
		api_out_message::send(connection, "LOAD FILE\n"
			"Block-code: " + block_code_.string('_') + "\n"
			"File-size: 0\n"
			"\n");
	}
}

// PEER INFO

api_out_peer_info::api_out_peer_info(local_peer &local_peer) :
api_out_message(local_peer) {
}

api_out_peer_info::~api_out_peer_info() {
}

void api_out_peer_info::send(api_connection::pointer connection) {
	api_out_message::send(connection, "PEER INFO\n"
		"Peer-code: " + local_peer_.code().string() + "\n"
		"Integrated: " + (local_peer_.integrated() ? "yes\n" : "no\n") +
		"Peers: " + boost::lexical_cast<string>(local_peer_.foreign_peers().size()) + "\n"
		"\n");
}