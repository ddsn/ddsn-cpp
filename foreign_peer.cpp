#include "foreign_peer.h"

#include <openssl/pem.h>

using namespace ddsn;
using namespace std;

foreign_peer::foreign_peer() : public_key_(nullptr), in_layer_(-1), out_layer_(-1), host_(""), port_(-1), integrated_(false) {

}

foreign_peer::~foreign_peer() {

}

const peer_id &foreign_peer::id() const {
	return id_;
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

int foreign_peer::in_layer() const {
	return in_layer_;
}

int foreign_peer::out_layer() const {
	return out_layer_;
}

bool foreign_peer::queued() const {
	return queued_;
}

bool foreign_peer::integrated() const {
	return integrated_;
}

void foreign_peer::set_id(const peer_id &id) {
	id_ = id;
}

const string &foreign_peer::host() const {
	return host_;
}

int foreign_peer::port() const {
	return port_;
}
bool foreign_peer::connected() const {
	return (bool)peer_connection_;
}

std::shared_ptr<peer_connection> foreign_peer::connection() const {
	return peer_connection_;
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

void foreign_peer::set_connection(peer_connection::pointer peer_connection) {
	peer_connection_ = peer_connection;
}

void foreign_peer::set_in_layer(int layer) {
	in_layer_ = layer;
}

void foreign_peer::set_out_layer(int layer) {
	out_layer_ = layer;
}

void foreign_peer::set_queued(bool queued) {
	queued_ = queued;
}

void foreign_peer::set_host(const std::string &host) {
	host_ = host;
}

void foreign_peer::set_port(int port) {
	port_ = port;
}

void foreign_peer::set_integrated(bool integrated) {
	integrated_ = integrated;
}