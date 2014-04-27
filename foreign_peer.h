#ifndef DDSN_FOREIGN_PEER_H
#define DDSN_FOREIGN_PEER_H

#include "peer_connection.h"
#include "peer_id.h"

namespace ddsn {

class peer_connection;

class foreign_peer {
public:
	foreign_peer();
	~foreign_peer();

	const peer_id &id() const;
	const std::string &public_key() const;
	bool identity_verified() const;

	void set_public_key(const std::string &public_key);
	void set_identity_verified(bool identity_verified);

	void set_id(const peer_id &id);
private:
	peer_id id_;
	peer_connection *peer_connection_;
	std::string public_key_;
	bool identity_verified_;
};

}

#endif