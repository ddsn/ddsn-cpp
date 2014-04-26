#include "code.h"

#include <openssl/sha.h>

#include <assert.h>

#include <cstring>
#include <memory>
#include <iostream>

using namespace ddsn;

code code::from_name(const std::string &name) {
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, name.c_str(), name.length());
	SHA256_Final(hash, &sha256);
	
	return code(SHA256_DIGEST_LENGTH, (char *)hash);
}

code::code() {
	code_ = new char[1];
	layers_ = 0;
	memset(code_, 0, 1);
}

code::code(int layers) {
	int size = (layers - 1) / 8 + 1;
	code_ = new char[size];
	layers_ = layers;
	memset(code_, 0, size);
}

code::code(int layers, const char *code) {
	int size = (layers - 1) / 8 + 1;
	code_ = new char[size];
	layers_ = layers;
	memcpy(code_, code, size);
}

code::code(std::string code, char delim) {
	int layers;
	const int code_length = code.length();

	int delim_pos = code.find(delim);

	if (delim_pos == std::string::npos) {
		layers = code_length * 4;
		delim_pos = code_length;
	} else {
		layers = delim_pos * 4 + (code_length - delim_pos - 1);
	}

	int size = (layers - 1) / 8 + 1;
	code_ = new char[size];
	layers_ = layers;
	memset(code_, 0, size);

	int l = 0;
	for (int i = 0; i < code_length; i++) {
		if (i > delim_pos) {
			set_layer_code(l++, code[i] == '0' ? 0 : 1);
		} else if (i < delim_pos) {
			if (code[i] >= '0' && code[i] <= '9') {
				int bits = code[i] - '0';
				set_layer_code(l++, (bits >> 3) & 1);
				set_layer_code(l++, (bits >> 2) & 1);
				set_layer_code(l++, (bits >> 1) & 1);
				set_layer_code(l++, bits & 1);
			} else if (code[i] >= 'a' && code[i] <= 'f') {
				int bits = code[i] - 'a' + 10;
				set_layer_code(l++, (bits >> 3) & 1);
				set_layer_code(l++, (bits >> 2) & 1);
				set_layer_code(l++, (bits >> 1) & 1);
				set_layer_code(l++, bits & 1);
			}
		}
	}
}

code::code(const code &code) {
	int size = (code.layers_ - 1) / 8 + 1;
	code_ = new char[size];
	layers_ = code.layers_;
	memcpy(code_, code.code_, size);
}

code::~code() {
	delete[] code_;
}

int code::layer_code(int layer) const {
	if (layer > layers_) return -1;
	return (code_[layer / 8] >> (layer % 8)) & 1;
}

int code::ext_layer_code(int layer) const {
	if (layer > layers_) return 0;
	return (code_[layer / 8] >> (layer % 8)) & 1;
}

void code::set_layer_code(int layer, int code) {
	assert(layer < layers_);
	if (code == 0) {
		code_[layer / 8] &= 0xFF ^ (1 << (layer % 8));
	}
	else {
		code_[layer / 8] |= 0x00 ^ (1 << (layer % 8));
	}
}

void code::resize_layers(int layers) {
	if (layers > layers_) {
		int oldSize = (layers_ - 1) / 8 + 1;
		int newSize = (layers - 1) / 8 + 1;

		if (newSize > oldSize) {
			char *newcode = new char[newSize];
			memcpy(newcode, code_, oldSize);

			delete[] code_;
			code_ = newcode;
		}

		layers_ = layers;
	}
}

int code::layers() const {
	return layers_;
}

bool code::contains(const code &code) const {
	for (int i = 0; i < layers_; i++) {
		if (code.ext_layer_code(i) != layer_code(i)) {
			return false;
		}
	}
	return true;
}

int code::differing_layer(const code &code) const {
	for (int i = 0; i < layers_; i++) {
		if (code.ext_layer_code(i) != layer_code(i)) {
			return i;
		}
	}
	return -1;
}

code &code::operator=(const code &code) {
	int size = (code.layers_ - 1) / 8 + 1;
	delete[] code_;
	code_ = new char[size];
	layers_ = code.layers_;
	memcpy(code_, code.code_, size);
	return *this;
}

std::string code::string(char delim) const {
	std::string string;
	int i = 0;
	for (; i + 3 < layers_; i += 4) {
		int digit = layer_code(i) << 3 | layer_code(i + 1) << 2 | layer_code(i + 2) << 1 | layer_code(i + 3);
		if (digit <= 9) {
			string += (char)('0' + digit);
		}
		else {
			string += (char)('a' + digit - 10);
		}
	}
	if (i < layers_) {
		string += delim;
		for (; i < layers_; i++) {
			string += layer_code(i) == 0 ? '0' : '1';
		}
	}
	return string;
}

namespace ddsn {
	std::ostream& operator<<(std::ostream& stream, const code& code) {
		stream << code.string();
		return stream;
	}
}
