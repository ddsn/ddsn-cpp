#include "api_messages.h"

#include "api_server.h"
#include "definitions.h"
#include "utilities.h"

#include <boost/bind.hpp>
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

api_out_message::api_out_message() {

}

api_out_message::~api_out_message() {

}

void api_out_message::send(api_connection::pointer connection, const std::string &string) {
	connection->send(string);
}

void api_out_message::send(api_connection::pointer connection, const BYTE *bytes, size_t size) {
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
		api_out_hello().send(connection_);

		connection_->set_authenticated(true);
		connection_->server().add_connection(connection_);

		type = DDSN_MESSAGE_TYPE_END;
	} else {
		type = DDSN_MESSAGE_TYPE_ERROR;
	}
}

void api_in_hello::feed(const BYTE *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}



// PING

api_in_ping::api_in_ping(local_peer &local_peer, api_connection::pointer connection) : api_in_message(local_peer, connection) {
	
}

api_in_ping::~api_in_ping() {

}

void api_in_ping::first_action(int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_END;

	api_out_ping().send(connection_);
}

void api_in_ping::feed(const string &line, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_ERROR;
}

void api_in_ping::feed(const BYTE *data, size_t size, int &type, size_t &expected_size) {
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

			data_ = new BYTE[file_size_];

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

				if (file_size_ > 8 * 1024 * 1024) {
					type = DDSN_MESSAGE_TYPE_ERROR;
					return;
				}
			} else if (field_name == "Chunks") {
				chunks_ = stoi(field_value);
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
			}

			type = DDSN_MESSAGE_TYPE_STRING;
		}
	}
}

static void action_api_store_block(api_connection::pointer connection, const block &block, bool success) {
	api_out_store_block(block, success).send(connection);
}

void api_in_store_file::feed(const BYTE *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_MESSAGE_TYPE_STRING;

	if (chunk_ < chunks_) {
		memcpy(data_ + data_pointer_, data, size);

		data_pointer_ += size;

		type = DDSN_MESSAGE_TYPE_STRING;
	} else {
		memcpy(data_ + data_pointer_, data, size);

		for (UINT32 occurrence = 0; occurrence < 4; occurrence++) {
			block block(file_name_);
			block.set_data(data_, file_size_);
			block.set_owner(local_peer_.keypair());
			block.set_occurrence(occurrence);
			block.seal();

			local_peer_.store(block, boost::bind(&action_api_store_block, connection_, _1, _2));
		}

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

static void action_api_load_block(api_connection::pointer connection, const block &block, bool success) {
	api_out_load_block(block, success).send(connection);
}

void api_in_load_file::feed(const string &line, int &type, size_t &expected_size) {
	if (line == "") {
		local_peer_.load(code_, boost::bind(&action_api_load_block, connection_, _1, _2));
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

void api_in_load_file::feed(const BYTE *data, size_t size, int &type, size_t &expected_size) {
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
		if (!local_peer_.integrated()) {
			local_peer_.connect(host_, port_, shared_ptr<foreign_peer>(new foreign_peer()), "queued");
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

		if (field_name == "Host") {
			host_ = field_value;
		} else if (field_name == "Port") {
			port_ = stoi(field_value);
		}
	}
}

void api_in_connect_peer::feed(const BYTE *data, size_t size, int &type, size_t &expected_size) {
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

void api_in_peer_info::feed(const BYTE *data, size_t size, int &type, size_t &expected_size) {
}

// PEER BLOCKS

api_in_peer_blocks::api_in_peer_blocks(local_peer &local_peer, api_connection::pointer connection) :
api_in_message(local_peer, connection) {

}

api_in_peer_blocks::~api_in_peer_blocks() {

}

void api_in_peer_blocks::first_action(int &type, size_t &expected_size) {
	api_out_peer_blocks(local_peer_).send(connection_);

	type = DDSN_MESSAGE_TYPE_END;
}

void api_in_peer_blocks::feed(const string &line, int &type, size_t &expected_size) {
}

void api_in_peer_blocks::feed(const BYTE *data, size_t size, int &type, size_t &expected_size) {
}

/*
  OUT MESSAGES
*/

// HELLO

api_out_hello::api_out_hello() {
}

api_out_hello::~api_out_hello() {
}

void api_out_hello::send(api_connection::pointer connection) {
	api_out_message::send(connection, "HELLO\n");
}

// PONG

api_out_ping::api_out_ping() {
}

api_out_ping::~api_out_ping() {
}

void api_out_ping::send(api_connection::pointer connection) {
	api_out_message::send(connection, "PONG\n");
}

// STORE BLOCK

api_out_store_block::api_out_store_block(const block &block, bool success) :
block_(block), success_(success) {
}

api_out_store_block::~api_out_store_block() {
}

void api_out_store_block::send(api_connection::pointer connection) {
	api_out_message::send(connection, "STORE BLOCK\n"
		"Code: " + block_.code().string() + "\n"
		"Name: " + block_.name() + "\n"
		"Occurrence: " + boost::lexical_cast<string>(block_.occurrence()) + "\n"
		"Success: " + (success_ ? "yes" : "no") + "\n"
		"\n");
}

// LOAD BLOCK

api_out_load_block::api_out_load_block(const block &block, bool success) :
block_(block), success_(success) {
}

api_out_load_block::~api_out_load_block() {
}

void api_out_load_block::send(api_connection::pointer connection) {
	api_out_message::send(connection, "LOAD BLOCK\n"
		"Code: " + block_.code().string('_') + "\n"
		"Name: " + block_.name() + "\n"
		"Owner: " + bytes_to_hex(block_.owner_hash(), 32) + "\n"
		"Size: " + boost::lexical_cast<string>(block_.size()) + "\n"
		"Success: " + (success_ ? "yes" : "no") + "\n"
		"\n");
}

// PEER INFO

api_out_peer_info::api_out_peer_info(const local_peer &local_peer) :
local_peer_(local_peer) {
}

api_out_peer_info::~api_out_peer_info() {
}

void api_out_peer_info::send(api_connection::pointer connection) {
	api_out_message::send(connection, "PEER INFO\n"
		"Peer-id: " + local_peer_.id().string() + "\n"
		"Peer-code: " + local_peer_.code().string() + "\n"
		"Integrated: " + (local_peer_.integrated() ? "yes" : "no") + "\n"
		"Mentor: " + (local_peer_.mentor() ? local_peer_.mentor()->id().string() : "") + "\n"
		"Blocks: " + boost::lexical_cast<string>(local_peer_.blocks()) + "\n"
		"Capacity: " + boost::lexical_cast<string>(local_peer_.capacity()) + "\n"
		"Peers: " + boost::lexical_cast<string>(local_peer_.foreign_peers().size()) + "\n\n");

	for (auto it = local_peer_.foreign_peers().begin(); it != local_peer_.foreign_peers().end(); ++it) {
		api_out_message::send(connection, "Peer-id: " + it->second->id().string() + "\n"
			"Connected: " + (it->second->connected() ? "yes" : "no") + "\n"
			"In-layer: " + boost::lexical_cast<string>(it->second->in_layer()) + "\n"
			"Out-layer: " + boost::lexical_cast<string>(it->second->out_layer()) + "\n\n");
	}

	api_out_message::send(connection, "\n");
}

// PEER BLOCKS

api_out_peer_blocks::api_out_peer_blocks(const local_peer &local_peer) :
local_peer_(local_peer) {
}

api_out_peer_blocks::~api_out_peer_blocks() {
}

void api_out_peer_blocks::send(api_connection::pointer connection) {
	api_out_message::send(connection, "PEER BLOCKS\n"
		"Blocks: " + boost::lexical_cast<string>(local_peer_.blocks()) + "\n\n");

	for (auto it = local_peer_.stored_blocks().begin(); it != local_peer_.stored_blocks().end(); ++it) {
		block stored_block(*it);
		stored_block.load_from_filesystem();

		BYTE hash[32];
		hash_from_rsa(stored_block.owner(), hash);

		api_out_message::send(connection, "Code: " + stored_block.code().string('_') + "\n"
			"Owner: " + bytes_to_hex(hash, 32) + "\n"
			"Name: " + stored_block.name() + "\n"
			"Size: " + boost::lexical_cast<string>(stored_block.size()) + "\n\n");
	}

	api_out_message::send(connection, "\n");
}