#include "api_server.h"

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
	std::cout << "waiting for new api connection..." << std::endl;

	api_connection::pointer new_connection = api_connection::create(io_service_);

	acceptor_.async_accept(new_connection->socket(),
		boost::bind(&api_server::handle_accept, this, new_connection,
		boost::asio::placeholders::error));
}

void api_server::handle_accept(api_connection::pointer new_connection, const error_code& error) {
	if (!error) {
		new_connection->start();
	}

	start_accept();
}

api_connection::api_connection(io_service &io_service) : socket_(io_service) {

}

api_connection::pointer api_connection::create(io_service &io_service) {
	return pointer(new api_connection(io_service));
}

tcp::socket &api_connection::socket() {
	return socket_;
}

void api_connection::start() {

}