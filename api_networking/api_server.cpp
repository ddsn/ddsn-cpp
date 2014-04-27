#include "api_server.h"
#include "api_connection.h"

#include <boost/bind.hpp>

#include <iostream>

using namespace ddsn;
using boost::asio::io_service;
using boost::asio::ip::tcp;

api_server::api_server(local_peer &local_peer, io_service &io_service, int port) :
local_peer_(local_peer), io_service_(io_service), port_(port), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {

}

api_server::~api_server() {

}

void api_server::start_accept() {
	api_connection::pointer new_connection = api_connection::pointer(new api_connection(local_peer_, io_service_));

	std::cout << "Waiting for connection API#" << new_connection->id() << " on port " << port_ << std::endl;

	acceptor_.async_accept(new_connection->socket(),
		boost::bind(&api_server::handle_accept, this, new_connection->shared_from_this(),
		boost::asio::placeholders::error));
}

void api_server::handle_accept(api_connection::pointer new_connection, const error_code& error) {
	if (!error) {
		new_connection->start();
	}

	start_accept();
}