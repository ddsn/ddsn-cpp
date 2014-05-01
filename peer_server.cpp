#include "peer_server.h"
#include "definitions.h"

#include <boost/bind.hpp>

using namespace ddsn;
using namespace std;
using boost::asio::io_service;
using boost::asio::ip::tcp;

peer_server::peer_server(local_peer &local_peer, io_service &io_service) :
local_peer_(local_peer), io_service_(io_service), port_(4494) {

}

peer_server::~peer_server() {
	delete acceptor_;
}

void peer_server::set_port(int port) {
	port_ = port;
}

void peer_server::start_accept() {
	acceptor_ = new tcp::acceptor(io_service_, tcp::endpoint(tcp::v4(), port_));

	next_accept();
}

void peer_server::next_accept() {
	peer_connection::pointer new_connection = peer_connection::pointer(
		new peer_connection(
			local_peer_, 
			io_service_));
	new_connection->set_foreign_peer(shared_ptr<foreign_peer>(new foreign_peer()));

	cout << "Waiting for connection PEER#" << new_connection->id() << " on port " << port_ << endl;

	acceptor_->async_accept(new_connection->socket(),
		boost::bind(&peer_server::handle_accept, this, new_connection->shared_from_this(),
		boost::asio::placeholders::error));
}

void peer_server::handle_accept(peer_connection::pointer new_connection, const boost::system::error_code& error) {
	if (!error) {
		new_connection->start();
	}

	cout << "PEER#" << new_connection->id() << " CONNECTED" << endl;

	next_accept();
}
