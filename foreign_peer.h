#ifndef DDSN_FOREIGN_H
#define DDSN_FOREIGN_H

#include "code.h"
#include "peer_connection.h"
#include "peer_id.h"

#include <openssl/rsa.h>
#include <memory>

namespace ddsn {

class peer_connection;

class foreign_peer {
public:
	foreign_peer();
	~foreign_peer();

	const peer_id &id() const;
	const std::string &public_key_str() const;
	RSA *public_key() const;
	bool identity_verified() const;
	int verification_number() const;
	int in_layer() const;
	int out_layer() const;
	bool queued() const;

	void set_id(const peer_id &id);
	void set_public_key_str(const std::string &public_key);
	void set_identity_verified(bool identity_verified);
	void set_verification_number(int verification_number);
	void set_connection(std::shared_ptr<peer_connection> peer_connection);
	void set_in_layer(int layer);
	void set_out_layer(int layer);
private:
	peer_id id_;
	std::shared_ptr<peer_connection> peer_connection_;
	std::string public_key_str_;
	RSA *public_key_;
	bool identity_verified_;
	int verification_number_;
	int in_layer_;
	int out_layer_;
};

}

#endif
