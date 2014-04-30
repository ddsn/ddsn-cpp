#ifndef DDSN_LOCAL_H
#define DDSN_LOCAL_H

#include "block.h"
#include "code.h"
#include "foreign_peer.h"
#include "peer_id.h"

#include <openssl/rsa.h>
#include <boost/asio.hpp>
#include <unordered_map>
#include <unordered_set>

namespace ddsn {

class foreign_peer;

class local_peer {
public:
	local_peer(boost::asio::io_service &io_service, std::string host, int port);
	~local_peer();

	const peer_id &id() const;
	const ddsn::code &code() const;
	bool integrated() const;
	int capacity() const;
	const std::string &host() const;
	int port() const;

	void set_integrated(bool integrated);
	void set_capacity(int capactiy);

	// keys
	int load_key();
	int save_key();
	void generate_key();
	RSA *keypair();

	void load_area_keys();

	// blocks
	void store(const block &block);
	void load(block &block);
	bool exists(const ddsn::code &code);

	// peers
	void connect(std::string host, int port);
	void add_foreign_peer(std::shared_ptr<foreign_peer> foreign_peer);
private:
	void create_id_from_key();

	peer_id id_;
	ddsn::code code_;
	boost::asio::io_service &io_service_;
	bool integrated_;
	int capacity_;
	int blocks_;
	std::unordered_set<ddsn::code> stored_blocks_;
	RSA *keypair_;
	std::unordered_map<peer_id, std::shared_ptr<foreign_peer>> foreign_peers_;
	std::string host_;
	int port_;
};

}

#endif
