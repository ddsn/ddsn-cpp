#ifndef DDSN_LOCAL_PEER_H
#define DDSN_LOCAL_PEER_H

#include "block.h"
#include "peer_id.h"

#include <openssl/rsa.h>

namespace ddsn {

class local_peer {
public:
	local_peer();
	~local_peer();

	const peer_id &id() const;
	const code &code() const;
	bool integrated() const;
	int capacity() const;

	void set_integrated(bool integrated);
	void set_capacity(int capactiy);

	int load_peer_key();
	int save_peer_key();
	void generate_peer_key();

	void load_area_keys();

	void store(const block &block);
	void load(block &block);
	bool exists(const ddsn::code &code);
private:
	void create_id_from_key();

	peer_id id_;
	ddsn::code code_;
	bool integrated_;
	int capacity_;
	int blocks_;
	RSA *keypair_;
};

}

#endif