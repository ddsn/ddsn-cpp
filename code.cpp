#include "code.h"

#include <assert.h>

#include <memory>
#include <iostream>

using namespace ddsn;

Code::Code() {
	code_ = new char[1];
	layers_ = 0;
	memset(code_, 0, 1);
}

Code::Code(int layers) {
	int size = (layers - 1) / 8 + 1;
	code_ = new char[size];
	layers_ = layers;
	memset(code_, 0, size);
}

Code::Code(int layers, const char *code) {
	int size = (layers - 1) / 8 + 1;
	code_ = new char[size];
	layers_ = layers;
	memcpy(code_, code, size);
}

Code::Code(std::string code) {
	int layers;
	const int code_length = code.length();

	int colon_pos = code.find(':');

	if (colon_pos == std::string::npos) {
		layers = code_length * 4;
		colon_pos = code_length;
	} else {
		layers = colon_pos * 4 + (code_length - colon_pos - 1);
	}

	int size = (layers - 1) / 8 + 1;
	code_ = new char[size];
	layers_ = layers;
	memset(code_, 0, size);

	int l = 0;
	for (int i = 0; i < code_length; i++) {
		if (i > colon_pos) {
			set_layer_code(l++, code[i] == '0' ? 0 : 1);
		} else if (i < colon_pos) {
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

Code::Code(const Code &code) {
	int size = (code.layers_ - 1) / 8 + 1;
	code_ = new char[size];
	layers_ = code.layers_;
	memcpy(code_, code.code_, size);
}

Code::~Code() {
	delete[] code_;
}

int Code::get_layer_code(int layer) const {
	if (layer > layers_) return -1;
	return (code_[layer / 8] >> (layer % 8)) & 1;
}

int Code::get_ext_layer_code(int layer) const {
	if (layer > layers_) return 0;
	return (code_[layer / 8] >> (layer % 8)) & 1;
}

void Code::set_layer_code(int layer, int code) {
	assert(layer < layers_);
	if (code == 0) {
		code_[layer / 8] &= 0xFF ^ (1 << (layer % 8));
	}
	else {
		code_[layer / 8] |= 0x00 ^ (1 << (layer % 8));
	}
}

void Code::resize_layers(int layers) {
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

int Code::get_layers() const {
	return layers_;
}

bool Code::contains(const Code &code) const {
	for (int i = 0; i < layers_; i++) {
		if (code.get_ext_layer_code(i) != get_layer_code(i)) {
			return false;
		}
	}
	return true;
}

int Code::get_differing_layer(const Code &code) const {
	for (int i = 0; i < layers_; i++) {
		if (code.get_ext_layer_code(i) != get_layer_code(i)) {
			return i;
		}
	}
	return -1;
}

Code &Code::operator=(const Code &code) {
	int size = (code.layers_ - 1) / 8 + 1;
	delete[] code_;
	code_ = new char[size];
	layers_ = code.layers_;
	memcpy(code_, code.code_, size);
	return *this;
}

std::string Code::get_string() const {
	std::string string;
	int i = 0;
	for (; i + 3 < layers_; i += 4) {
		int digit = get_layer_code(i) << 3 | get_layer_code(i + 1) << 2 | get_layer_code(i + 2) << 1 | get_layer_code(i + 3);
		if (digit <= 9) {
			string += (char)('0' + digit);
		} else {
			string += (char)('a' + digit - 10);
		}
	}
	if (i < layers_) {
		string += ':';
		for (; i < layers_; i++) {
			string += get_layer_code(i) == 0 ? '0' : '1';
		}
	}
	return string;
}

namespace ddsn {
	std::ostream& operator<<(std::ostream& stream, const Code& code) {
		stream << code.get_string();
		return stream;
	}
}