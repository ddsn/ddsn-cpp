#ifndef DDSN_LOCAL_PEER_H
#define DDSN_LOCAL_PEER_H

#include "block.h"
#include "foreign_peer.h"
#include "peer_id.h"

#include <openssl/rsa.h>
#include <boost/asio.hpp>
#include <unordered_map>

namespace ddsn {

class foreign_peer;

class local_peer {
public:
	local_peer(boost::asio::io_service &io_service);
	~local_peer();

	const peer_id &id() const;
	const ddsn::code &code() const;
	bool integrated() const;
	int capacity() const;

	void set_integrated(bool integrated);
	void set_capacity(int capactiy);

	// keys
	int load_peer_key();
	int save_peer_key();
	void generate_peer_key();
	RSA *keypair();

	void load_area_keys();

	// blocks
	void store(const block &block);
	void load(block &block);
	bool exists(const ddsn::code &code);

	// peers
	void connect(std::string host, int port);
private:
	void create_id_from_key();

	peer_id id_;
	ddsn::code code_;
	boost::asio::io_service &io_service_;
	bool integrated_;
	int capacity_;
	int blocks_;
	RSA *keypair_;
	std::unordered_map<peer_id, foreign_peer *> foreign_peers_;
};

}

#endif
