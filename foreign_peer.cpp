#include "foreign_peer.h"

#include <openssl/pem.h>

using namespace ddsn;

foreign_peer::foreign_peer() : public_key_(nullptr) {

}

foreign_peer::~foreign_peer() {

}

const peer_id &foreign_peer::id() const {
	return id_;
}

void foreign_peer::set_id(const peer_id &id) {
	id_ = id;
}

const std::string &foreign_peer::public_key_str() const {
	return public_key_str_;
}

RSA *foreign_peer::public_key() const {
	return public_key_;
}

bool foreign_peer::identity_verified() const {
	return identity_verified_;
}

int foreign_peer::verification_number() const {
	return verification_number_;
}

void foreign_peer::set_public_key_str(const std::string &public_key) {
	public_key_str_ = public_key;

	BIO *pub = BIO_new(BIO_s_mem());

	BIO_write(pub, public_key_str_.c_str(), public_key_str_.length());

	PEM_read_bio_RSAPublicKey(pub, &public_key_, NULL, NULL);
}

void foreign_peer::set_identity_verified(bool identity_verified) {
	identity_verified_ = identity_verified;
}

void foreign_peer::set_verification_number(int verification_number) {
	verification_number_ = verification_number;
}