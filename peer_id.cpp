#include "peer_id.h"
#include "utilities.h"

#include <cstring>
#include <iostream>
#include <memory>

using namespace ddsn;
using namespace std;

peer_id::peer_id() {

}

peer_id::peer_id(BYTE id[32]) {
	memcpy(id_, id, 32);
}

peer_id::~peer_id() {

}

const BYTE *peer_id::id() const {
	return id_;
}

void peer_id::set_id(BYTE id[32]) {
	memcpy(id_, id, 32);
}

peer_id &peer_id::operator=(const peer_id &peer_id) {
	memcpy(id_, peer_id.id_, 32);
	return *this;
}

bool peer_id::operator==(const peer_id &peer_id) const {
	return memcmp(id_, peer_id.id_, 32) == 0;
}

string peer_id::string() const {
	return bytes_to_hex(id_, 32);
}

string peer_id::short_string() const {
	return bytes_to_hex(id_, 3);
}

namespace ddsn {
	ostream& operator<<(ostream& stream, const peer_id& code) {
		stream << code.string();
		return stream;
	}
}

size_t hash<peer_id>::operator()(const peer_id &peer_id) const {
	const BYTE *id_ = peer_id.id();
	UINT32 hash = 0;

	for (int i = 0; i < 32; i += 4) {
		hash ^= (UINT32)id_[i] << 24 | (UINT32)id_[i + 1] << 16 | (UINT32)id_[i + 2] << 8 | (UINT32)id_[i + 3];
	}

	return hash;
}
