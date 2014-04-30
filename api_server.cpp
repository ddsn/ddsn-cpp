#include "api_server.h"

#include "api_connection.h"

#include <boost/bind.hpp>
#include <iostream>

using namespace ddsn;
using namespace std;
using boost::asio::ip::tcp;

api_server::api_server(local_peer &local_peer, io_service &io_service, const string &password) :
local_peer_(local_peer), io_service_(io_service), password_(password), port_(4495) {

}

api_server::~api_server() {
	delete acceptor_;
}

string api_server::password() {
	return password_;
}

void api_server::set_port(int port) {
	port_ = port;
}

void api_server::start_accept() {
	acceptor_ = new tcp::acceptor(io_service_, tcp::endpoint(tcp::v4(), port_));

	next_accept();
}


void api_server::next_accept() {
	api_connection::pointer new_connection = api_connection::pointer(new api_connection(*this, local_peer_, io_service_));

	cout << "Waiting for connection API#" << new_connection->id() << " on port " << port_ << endl;

	acceptor_->async_accept(new_connection->socket(),
		boost::bind(&api_server::handle_accept, this, new_connection->shared_from_this(),
		boost::asio::placeholders::error));
}

void api_server::handle_accept(api_connection::pointer new_connection, const boost::system::error_code& error) {
	if (!error) {
		new_connection->start();
	}

	next_accept();
}

void api_server::add_connection(api_connection::pointer connection) {
	connections_.push_back(connection);
}

void api_server::remove_connection(api_connection::pointer connection) {
	connections_.remove(connection);
}

void api_server::broadcast(api_out_message &api_out_message) {
	for (auto it = connections_.begin(); it != connections_.end(); ++it) {
		api_out_message.send(*it);
	}
}