#ifndef DDSN_FOREIGN_H
#define DDSN_FOREIGN_H

#include "code.h"
#include "peer_connection.h"
#include "peer_id.h"

#include <openssl/rsa.h>
#include <memory>
#include <string>

namespace ddsn {

class peer_connection;

class foreign_peer {
public:
	foreign_peer();
	~foreign_peer();

	const peer_id &id() const;
	const std::string &host() const;
	INT32 port() const;
	const std::string &public_key_str() const;
	RSA *public_key() const;

	bool integrated() const;
	bool identity_verified() const;

	INT32 in_layer() const;
	INT32 out_layer() const;
	bool queued() const;

	bool connected() const;
	std::shared_ptr<peer_connection> connection() const;

	int verification_number() const;

	void set_id(const peer_id &id);
	void set_host(const std::string &host);
	void set_port(INT32 port);
	void set_public_key_str(const std::string &public_key);

	void set_integrated(bool integrated);
	void set_identity_verified(bool identity_verified);

	void set_in_layer(INT32 layer);
	void set_out_layer(INT32 layer);
	void set_queued(bool queued);

	void set_connection(std::shared_ptr<peer_connection> peer_connection);
	void set_verification_number(int verification_number);
private:
	peer_id id_;
	std::string host_;
	INT32 port_;
	std::string public_key_str_;
	RSA *public_key_;

	bool integrated_;
	bool identity_verified_;

	INT32 in_layer_;
	INT32 out_layer_;
	bool queued_;

	std::shared_ptr<peer_connection> peer_connection_;
	UINT32 verification_number_;
};

}

#endif
