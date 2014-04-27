#include "peer_id.h"

#include <iostream>
#include <memory>

using namespace ddsn;
using namespace std;

peer_id::peer_id() {

}

peer_id::peer_id(unsigned char id[32]) {
	memcpy(id_, id, 32);
}

peer_id::~peer_id() {

}

const unsigned char *peer_id::id() const {
	return id_;
}

void peer_id::set_id(unsigned char id[32]) {
	memcpy(id_, id, 32);
}

peer_id &peer_id::operator=(const peer_id &peer_id) {
	memcpy(id_, peer_id.id_, 32);
	return *this;
}

string peer_id::string() const {
	std::string string;
	for (int i = 0; i < 32; i += 1) {
		int digit = id_[i] >> 4;
		if (digit <= 9) {
			string += (char)('0' + digit);
		}
		else {
			string += (char)('a' + digit - 10);
		}

		digit = id_[i] & 0xF;
		if (digit <= 9) {
			string += (char)('0' + digit);
		}
		else {
			string += (char)('a' + digit - 10);
		}
	}
	return string;
}

namespace ddsn {
	ostream& operator<<(ostream& stream, const peer_id& code) {
		stream << code.string();
		return stream;
	}
}
