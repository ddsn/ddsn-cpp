#include "block.h"

#include <openssl/sha.h>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace ddsn;
using namespace std;

block::block(const string &name) : name_(name), data_(nullptr) {
}

block::block(const ddsn::code &code) : code_(code), data_(nullptr) {
}

block::~block() {
	if (data_ != nullptr) {
		delete data_;
	}
}

char *block::data() {
	return data_;
}

const code &block::code() const {
	return code_;
}

const std::string &block::name() const {
	return name_;
}

size_t block::size() const {
	return size_;
}

void block::set_data(const char *data, size_t size) {
	data_ = new char[size];
	size_ = size;

	memcpy(data_, data, size);

	// build code

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, data_, size);
	SHA256_Update(&sha256, name_.c_str(), name_.length());
	SHA256_Final(hash, &sha256);

	code_ = ddsn::code(SHA256_DIGEST_LENGTH * 8, hash);
}

int block::save_to_filesystem() const {
	if (code_.layers() != 256) {
		return -1;
	}

	ofstream file("blocks/" + code_.string('_'), ios::out | ios::binary);

	if (file.is_open()) {
		file.seekp(0, ios::beg);
		file.write(name_.c_str(), name_.length() + 1);
		file.write(data_, size_);
		file.close();

		return 0;
	} else {
		return 1;
	}
}

int block::load_from_filesystem() {
	if (code_.layers() != 256) {
		return -1;
	}

	ifstream file("blocks/" + code_.string('_'), ios::in | ios::binary | ios::ate);

	if (file.is_open()) {
		size_t data_size = (size_t)file.tellg();

		if (data_size == 0) {
			return -1;
		}

		file.seekg(0, ios::beg);

		name_ = "";

		char c;
		while (file.good() && (c = file.get()) != '\0') {
			name_ += c;
			data_size--;
		}
		data_size--; // for 0-character

		if (data_ != nullptr) {
			delete data_;
		}

		data_ = new char[data_size];
		size_ = data_size;

		file.read(data_, data_size);
		file.close();

		return 0;
	} else {
		return 1;
	}
}
