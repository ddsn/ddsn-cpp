#ifndef DDSN_LOCAL_H
#define DDSN_LOCAL_H

#include "block.h"
#include "code.h"
#include "foreign_peer.h"
#include "peer_id.h"

#include <openssl/rsa.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace ddsn {

class local_peer;

void action_peer_stored_block(local_peer &local_peer, const block &block, const ddsn::code &code, bool success);

class api_server;
class foreign_peer;

class local_peer {
public:
	local_peer(boost::asio::io_service &io_service, std::string host, int port);
	~local_peer();

	void set_api_server(ddsn::api_server *api_server);
	ddsn::api_server *api_server() const;

	// general
	const peer_id &id() const;
	const ddsn::code &code() const;
	const std::string &host() const;
	int port() const;
	void set_code(const ddsn::code &code);

	// keys
	int load_key();
	int save_key();
	void generate_key();
	RSA *keypair();
	void load_area_keys();

	// blocks
	void store(const block &block, boost::function<void(const ddsn::code&, const std::string &, bool)> action);
	void load(const ddsn::code &code, boost::function<void(block&)> action);
	bool exists(const ddsn::code &code);
	void redistribute_block();

	int capacity() const;
	int blocks() const;
	void set_capacity(int capactiy);

	void do_load_actions(block &block);
	void do_store_actions(const ddsn::code &code, const std::string &name, bool success);

	// network management
	bool integrated() const;
	bool splitting() const;
	std::shared_ptr<foreign_peer> mentor() const;
	void set_integrated(bool integrated);
	void set_splitting(bool splitting);
	void set_mentor(std::shared_ptr<foreign_peer> mentor);

	// peers
	void connect(std::string host, int port, std::shared_ptr<foreign_peer> foreign_peer, std::string type);
	void add_foreign_peer(std::shared_ptr<foreign_peer> foreign_peer);
	const std::unordered_map<peer_id, std::shared_ptr<foreign_peer>> &foreign_peers() const;
	std::shared_ptr<foreign_peer> connected_queued_peer() const;
private:
	std::shared_ptr<foreign_peer> out_peer(int layer, bool connected = true) const;

	boost::asio::io_service &io_service_;
	ddsn::api_server *api_server_;

	peer_id id_;
	ddsn::code code_;
	std::string host_;
	int port_;

	RSA *keypair_;

	UINT32 capacity_;
	std::unordered_set<ddsn::code> stored_blocks_;
	std::list<std::pair<ddsn::code, boost::function<void(block&)>>> load_actions_;
	std::list<std::pair<ddsn::code, boost::function<void(const ddsn::code&, const std::string &, bool)>>> store_actions_;

	bool integrated_;
	bool splitting_;
	std::shared_ptr<foreign_peer> mentor_;

	std::unordered_map<peer_id, std::shared_ptr<foreign_peer>> foreign_peers_;

	friend void ddsn::action_peer_stored_block(local_peer &local_peer, const block &block, const ddsn::code &code, bool success);
};

}

#endif
