#include "local_peer.h"

#include "api_server.h"
#include "peer_connection.h"
#include "peer_messages.h"
#include "utilities.h"

#include <openssl/pem.h>
#include <openssl/sha.h>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iostream>

using namespace ddsn;
using namespace std;
using boost::asio::ip::tcp;

local_peer::local_peer(io_service &io_service, string host, int port) :
io_service_(io_service), integrated_(false), keypair_(nullptr), host_(host), port_(port) {

}

local_peer::~local_peer() {

}

void local_peer::set_api_server(ddsn::api_server *api_server) {
	api_server_ = api_server;
}

api_server * local_peer::api_server() const {
	return api_server_;
}

const peer_id &local_peer::id() const {
	return id_;
}

const code &local_peer::code() const {
	return code_;
}

void local_peer::set_code(const ddsn::code &code) {
	code_ = code;
}

const string &local_peer::host() const {
	return host_;
}

int local_peer::port() const {
	return port_;
}

// keys

int local_peer::load_key() {
	BIO *pri = BIO_new(BIO_s_mem());

	ifstream pri_file("keys/local.pem", ios::in | ios::binary | ios::ate);

	if (pri_file.is_open()) {
		size_t pri_len = (size_t)pri_file.tellg();

		char *pri_key = new char[pri_len];

		pri_file.seekg(0, ios::beg);
		pri_file.read(pri_key, pri_len);
		pri_file.close();

		BIO_write(pri, pri_key, pri_len);

		PEM_read_bio_RSAPrivateKey(pri, &keypair_, NULL, NULL);

		delete[] pri_key;
	} else {
		return 1;
	}

	BYTE hash[32];
	hash_from_rsa(keypair_, hash);
	id_.set_id(hash);

	return 0;
}

int local_peer::save_key() {
	BIO *pri = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPrivateKey(pri, keypair_, NULL, NULL, 0, NULL, NULL);

	size_t pri_len = BIO_pending(pri);

	char *pri_key = new char[pri_len];

	BIO_read(pri, pri_key, pri_len);

	ofstream pri_file("keys/local.pem", ios::out | ios::binary);

	if (pri_file.is_open()) {
		pri_file.write(pri_key, pri_len);
		pri_file.close();
	} else {
		return 1;
	}

	delete[] pri_key;

	return 0;
}

void local_peer::generate_key() {
	keypair_ = RSA_generate_key(2048, 3, NULL, NULL);

	BYTE hash[32];
	hash_from_rsa(keypair_, hash);
	id_.set_id(hash);
}

void local_peer::load_area_keys() {

}

RSA *local_peer::keypair() {
	return keypair_;
}

// blocks

void local_peer::store(const block &block, boost::function<void(const ddsn::block &, bool)> action) {
	if (!integrated_) {
		return;
	}

	if (code_.contains(block.code())) {
		cout << "Save " << block.code().string('_') << " to filesystem" << endl;
		if (block.save_to_filesystem() == 0) {
			action(block, true);

			stored_blocks_.insert(block.code());

			if (stored_blocks_.size() > capacity_ && !splitting_) {
				// we have too many blocks and we are not splitting right now (i.e. nothing's be done about that yet)
				cout << "Capacity exhausted (" << stored_blocks_.size() << " blocks stored, capacity: " << capacity_ << ")" << endl;

				shared_ptr<foreign_peer> peer = connected_queued_peer();
				if (peer) {
					peer->set_queued(false);
					splitting_ = true;

					// generate new peer code with a trailing 1
					ddsn::code new_code = code_;
					int layers = new_code.layers();
					new_code.resize_layers(layers + 1);
					new_code.set_layer_code(layers, 1);

					peer_set_code(*this, peer->connection(), new_code).send();
				}
			}
		} else {
			action(block, false);
		}
	} else {
		int layer = code_.differing_layer(block.code());
		auto peer = out_peer(layer, true);

		store_actions_.push_back(std::pair<ddsn::code, boost::function<void(const ddsn::block &, bool)>>(block.code(), action));

		peer_store_block(*this, peer->connection(), block).send();
	}
}

void local_peer::load(const ddsn::code &block_code, boost::function<void(const block &, bool)> action) {
	if (!integrated_) {
		return;
	}

	if (code_.contains(block_code)) {
		cout << "Load " << block_code.string('_') << " from filesystem" << endl;
		
		block block(block_code);
		
		if (block.load_from_filesystem() == 0) {
			action(block, true);
		} else {
			action(block, false);
		}
	} else {
		int layer = code_.differing_layer(block_code);
		auto peer = out_peer(layer, true);

		load_actions_.push_back(std::pair<ddsn::code, boost::function<void(const block &, bool)>>(block_code, action));

		peer_load_block(*this, peer->connection(), block_code).send();
	}
}

int local_peer::blocks() const {
	return stored_blocks_.size();
}

std::unordered_set<ddsn::code> local_peer::stored_blocks() const {
	return stored_blocks_;
}

int local_peer::capacity() const {
	return capacity_;
}

void local_peer::set_capacity(int capacity) {
	capacity_ = capacity;
}

void ddsn::action_peer_stored_block(local_peer &local_peer, const block &block, bool success) {
	if (success) {
		block.delete_from_filesystem();
		local_peer.stored_blocks_.erase(block.code());
		local_peer.redistribute_block();
	}
}

void local_peer::redistribute_block() {
	for (auto it = stored_blocks_.begin(); it != stored_blocks_.end(); ++it) {
		if (!code_.contains(*it)) {
			block block(*it);
			block.load_from_filesystem();

			store(block, boost::bind(&action_peer_stored_block, boost::ref(*this), _1, _2));
			return;
		}
	}
	splitting_ = false;
}

void local_peer::do_load_actions(const block &block, bool success) {
	for (auto it = load_actions_.begin(); it != load_actions_.end();) {
		if (it->first == block.code()) {
			it->second(block, success);
			it = load_actions_.erase(it);
		} else {
			++it;
		}
	}
}

void local_peer::do_store_actions(const block &block, bool success) {
	for (auto it = store_actions_.begin(); it != store_actions_.end();) {
		if (it->first == block.code()) {
			it->second(block, success);
			it = store_actions_.erase(it);
		} else {
			++it;
		}
	}
}

// network management

bool local_peer::integrated() const {
	return integrated_;
}

bool local_peer::splitting() const {
	return splitting_;
}

std::shared_ptr<foreign_peer> local_peer::mentor() const {
	return mentor_;
}

void local_peer::set_splitting(bool splitting) {
	splitting_ = splitting;
}

void local_peer::set_integrated(bool integrated) {
	integrated_ = integrated;
}

void local_peer::set_mentor(std::shared_ptr<foreign_peer> mentor) {
	mentor_ = mentor;
}

// peers

void local_peer::connect(string host, int port, shared_ptr<foreign_peer> foreign_peer, string type) {
	cout << "Connecting to " << host << ":" << port << "..." << endl;

	tcp::resolver resolver(io_service_);
	tcp::resolver::query query(host, boost::lexical_cast<string>(port), boost::asio::ip::resolver_query_base::numeric_service);

	try {
		tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

		peer_connection::pointer new_connection(new peer_connection(*this, io_service_));
		new_connection->set_foreign_peer(foreign_peer);

		boost::asio::connect(new_connection->socket(), endpoint_iterator);

		cout << "PEER#" << new_connection->id() << " CONNECTED" << endl;

		peer_hello(*this, new_connection, type).send();

		new_connection->set_introduced(true);
		new_connection->start();
	} catch (...) {
		cout << "Could not connect to " << host << ":" << port << endl;
	}
}

void local_peer::add_foreign_peer(std::shared_ptr<foreign_peer> foreign_peer) {
	foreign_peers_[foreign_peer->id()] = foreign_peer;
	cout << "Added peer " << foreign_peer->id().short_string() << "; having " << foreign_peers_.size() << " peers now" << endl;

	if (foreign_peer->identity_verified() && foreign_peer->connected() && foreign_peer->out_layer() != -1) {
		// this peer is a peer we should connect to, because the out_layer is set
		int layer = foreign_peer->out_layer();

		// calculate code to show we are a valid in peer for this layer
		ddsn::code code;
		code.resize_layers(layer);
		for (int i = 0; i < layer; i++) {
			code.set_layer_code(i, code_.layer_code(i)); // TODO: implement a faster function for that in ddsn::code
		}

		peer_connect(*this, foreign_peer->connection(), layer, code).send();

		bool ready_for_integration = true;
		for (UINT32 i = 0; i < code_.layers(); i++) {
			if (!out_peer(i)) {
				ready_for_integration = false;
				break;
			}
		}

		if (ready_for_integration) {
			integrated_ = true;
			peer_integrated(*this, mentor_->connection()).send();
		}
	} else if (foreign_peer->identity_verified() && foreign_peer->connected() && foreign_peer->out_layer() == -1 && foreign_peer->in_layer() == -1) {
		if (stored_blocks_.size() > capacity_) {
			foreign_peer->set_queued(false);
			splitting_ = true;

			// generate new peer code with a trailing 1
			ddsn::code new_code = code_;
			int layers = new_code.layers();
			new_code.resize_layers(layers + 1);
			new_code.set_layer_code(layers, 1);

			peer_set_code(*this, foreign_peer->connection(), new_code).send();
		}
	}
}

const std::unordered_map<peer_id, std::shared_ptr<foreign_peer>> &local_peer::foreign_peers() const {
	return foreign_peers_;
}

std::shared_ptr<foreign_peer> local_peer::connected_queued_peer() const {
	for (auto it = foreign_peers_.begin(); it != foreign_peers_.end(); ++it) {
		if (it->second->connected() && it->second->queued() && it->second->identity_verified()) {
			return it->second;
			break;
		}
	}
	return nullptr;
}

std::shared_ptr<foreign_peer> local_peer::out_peer(int layer, bool connected) const {
	for (auto it = foreign_peers_.begin(); it != foreign_peers_.end(); ++it) {
		if ((!connected || it->second->connected()) && it->second->out_layer() == layer && it->second->identity_verified()) {
			return it->second;
			break;
		}
	}
	return nullptr;
}
