#include "api_messages.h"

#include "definitions.h"

#include <boost/lexical_cast.hpp>

using namespace ddsn;
using namespace std;

api_message *api_message::create_message(local_peer &local_peer, api_connection &connection, const string &first_line) {
	if (first_line == "PING") {
		return new api_ping(local_peer, connection);
	} else if (first_line == "STORE FILE") {
		return new api_store_file(local_peer, connection);
	} else if (first_line == "LOAD FILE") {
		return new api_load_file(local_peer, connection);
	}

	return nullptr;
}

api_message::api_message(local_peer &local_peer, api_connection &connection) : local_peer_(local_peer), connection_(connection) {

}

api_message::~api_message() {

}

// PING

api_ping::api_ping(local_peer &local_peer, api_connection &connection) : api_message(local_peer, connection) {
	
}

api_ping::~api_ping() {

}

void api_ping::first_action(int &type, size_t &expected_size) {
	type = DDSN_API_MESSAGE_TYPE_END;

	connection_.send("PONG\n");
}

void api_ping::feed(const string &line, int &type, size_t &expected_size) {
	type = DDSN_API_MESSAGE_TYPE_ERROR;
}

void api_ping::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_API_MESSAGE_TYPE_ERROR;
}

// STORE FILE

api_store_file::api_store_file(local_peer &local_peer, api_connection &connection) :
api_message(local_peer, connection), state_(0), chunk_(0), data_pointer_(0) {
	
}

api_store_file::~api_store_file() {

}

void api_store_file::first_action(int &type, size_t &expected_size) {
	type = DDSN_API_MESSAGE_TYPE_STRING;
}

void api_store_file::feed(const string &line, int &type, size_t &expected_size) {
	if (state_ == 0) { // File information
		if (line == "") { // End of file information
			state_ = 1;

			data_ = new char[file_size_];

			type = DDSN_API_MESSAGE_TYPE_STRING;
		} else {
			int colon_pos = line.find(": ");
			string field_name = line.substr(0, colon_pos);
			string field_value = line.substr(colon_pos + 2);

			if (field_name == "File-name") {
				file_name_ = field_value;
			} else if (field_name == "File-size") {
				file_size_ = stoi(field_value);

				if (file_size_ >= 8 * 1024 * 1024) {
					type = DDSN_API_MESSAGE_TYPE_ERROR;
					return;
				}
			} else if (field_name == "Chunks") {
				chunks_ = stoi(field_value);
			} else {
				// unknown field
			}

			type = DDSN_API_MESSAGE_TYPE_STRING;
		}
	} else if (state_ == 1) { // Chunk information
		if (line == "") {
			chunk_++;

			type = DDSN_API_MESSAGE_TYPE_BYTES;
			expected_size = chunk_size_;
		} else {
			int colon_pos = line.find(": ");
			string field_name = line.substr(0, colon_pos);
			string field_value = line.substr(colon_pos + 2);

			if (field_name == "Chunk-size") {
				chunk_size_ = stoi(field_value);
			} else {
				// unknown field
			}

			type = DDSN_API_MESSAGE_TYPE_STRING;
		}
	}
}

void api_store_file::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_API_MESSAGE_TYPE_STRING;

	if (chunk_ < chunks_) {
		memcpy(data_ + data_pointer_, data, size);

		data_pointer_ += size;

		type = DDSN_API_MESSAGE_TYPE_STRING;
	}
	else {
		memcpy(data_ + data_pointer_, data, size);

		block block(file_name_);
		block.set_data(data_, file_size_);

		local_peer_.store(block);

		connection_.send("FILE STORED\n"
			"Block-code: " + block.code().string() + "\n"
			"\n");

		type = DDSN_API_MESSAGE_TYPE_END;
	}
}

// LOAD FILE

api_load_file::api_load_file(local_peer &local_peer, api_connection &connection) :
	api_message(local_peer, connection) {

}

api_load_file::~api_load_file() {

}

void api_load_file::first_action(int &type, size_t &expected_size) {
	type = DDSN_API_MESSAGE_TYPE_STRING;
}

void api_load_file::feed(const string &line, int &type, size_t &expected_size) {
	if (line == "") {
		block block(code_);
		local_peer_.load(block);

		if (block.size() > 0) {
			connection_.send("FILE LOADED\n"
				"File-size: " + boost::lexical_cast<string>(block.size()) + "\n"
				"Block-code: " + code_.string('_') + "\n"
				"\n");
			connection_.send(block.data(), block.size());
		} else {
			connection_.send("FILE LOAD FAILED\n"
				"Block-code: " + code_.string('_') + "\n"
				"\n");
		}

		type = DDSN_API_MESSAGE_TYPE_END;
	} else {
		int colon_pos = line.find(": ");
		string field_name = line.substr(0, colon_pos);
		string field_value = line.substr(colon_pos + 2);

		if (field_name == "File-name") {
			code_ = code::from_name(field_value);
			type = DDSN_API_MESSAGE_TYPE_STRING;
		} else if (field_name == "Block-code") {
			code_ = code(field_value);
			type = DDSN_API_MESSAGE_TYPE_STRING;
		}
	}
}

void api_load_file::feed(const char *data, size_t size, int &type, size_t &expected_size) {
	type = DDSN_API_MESSAGE_TYPE_ERROR;
}