#include "foreign_peer.h"

using namespace ddsn;

foreign_peer::foreign_peer() {

}

foreign_peer::~foreign_peer() {

}

const peer_id &foreign_peer::id() const {
	return id_;
}

void foreign_peer::set_id(const peer_id &id) {
	id_ = id;
}

const std::string &foreign_peer::public_key() const {
	return public_key_;
}

bool foreign_peer::identity_verified() const {
	return identity_verified_;
}

void foreign_peer::set_public_key(const std::string &public_key) {
	public_key_ = public_key;
}

void foreign_peer::set_identity_verified(bool identity_verified) {
	identity_verified_ = identity_verified;
}