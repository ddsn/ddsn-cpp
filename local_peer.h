#ifndef DDSN_LOCAL_H
#define DDSN_LOCAL_H

#include "block.h"
#include "code.h"
#include "foreign_peer.h"
#include "peer_id.h"

#include <openssl/rsa.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <unordered_map>
#include <unordered_set>

namespace ddsn {

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
	void store(const block &block);
	void load(const ddsn::code &code, boost::function<void(block&)> function);
	bool exists(const ddsn::code &code);
	int capacity() const;
	int blocks() const;
	void set_capacity(int capactiy);
	void do_load_actions(block &block) const;

	// network management
	bool integrated() const;
	bool splitting() const;
	void set_integrated(bool integrated);
	void set_splitting(bool splitting);

	// peers
	void connect(std::string host, int port, std::shared_ptr<foreign_peer> foreign_peer, std::string type);
	void add_foreign_peer(std::shared_ptr<foreign_peer> foreign_peer);
	const std::unordered_map<peer_id, std::shared_ptr<foreign_peer>> &foreign_peers() const;
	std::shared_ptr<foreign_peer> connected_queued_peer() const;
private:
	void create_id_from_key();

	std::shared_ptr<foreign_peer> out_peer(int layer, bool connected = true) const;

	boost::asio::io_service &io_service_;
	ddsn::api_server *api_server_;

	peer_id id_;
	ddsn::code code_;
	std::string host_;
	int port_;

	RSA *keypair_;

	int capacity_;
	int blocks_;
	std::unordered_set<ddsn::code> stored_blocks_;

	bool integrated_;
	bool splitting_;

	std::unordered_map<peer_id, std::shared_ptr<foreign_peer>> foreign_peers_;
	std::list<std::pair<ddsn::code, boost::function<void(block&)>>> load_actions_;
};

}

#endif
