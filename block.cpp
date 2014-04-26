#include "block.h"

#include <fstream>
#include <iostream>

using namespace ddsn;
using namespace std;

block::block(const string &name) : name_(name), data_(nullptr) {
	code_ = code::from_name(name_);
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

size_t block::size() const {
	return size_;
}

void block::set_data(const char *data, size_t size) {
	data_ = new char[size];
	size_ = size;

	memcpy(data_, data, size);
}

int block::save_to_filesystem() const {
	ofstream file("blocks/" + code_.string('_'), ios::out | ios::binary);

	if (file.is_open()) {
		file.seekp(0, ios::beg);
		file.write(data_, size_);
		file.close();

		return 0;
	} else {
		return 1;
	}
}

int block::load_from_filesystem() {
	ifstream file("blocks/" + code_.string('_'), ios::in | ios::binary | ios::ate);

	if (file.is_open()) {
		size_t file_size = file.tellg();

		if (data_ != nullptr) {
			delete data_;
		}

		data_ = new char[file_size];
		size_ = file_size;

		file.seekg(0, ios::beg);
		file.read(data_, file_size);
		file.close();

		return 0;
	} else {
		return 1;
	}
}