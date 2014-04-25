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

	api_connection *new_connection = new api_connection(io_service_);

	acceptor_.async_accept(new_connection->socket(),
		boost::bind(&api_server::handle_accept, this, new_connection,
		boost::asio::placeholders::error));
}

void api_server::handle_accept(api_connection *new_connection, const error_code& error) {
	if (!error) {
		new_connection->start();
	}

	start_accept();
}

api_connection::api_connection(io_service &io_service) : socket_(io_service) {

}

tcp::socket &api_connection::socket() {
	return socket_;
}

void api_connection::start() {
	boost::asio::async_read_until(socket_, streambuf_, "\n", boost::bind(&api_connection::handle_read_line, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void api_connection::handle_read_line(const boost::system::error_code& error, std::size_t bytes_transferred) {
	if (!error && bytes_transferred) {
		std::cout << bytes_transferred << std::endl;

		streambuf_.commit(bytes_transferred);

		std::istream istream(&streambuf_);
		std::string string;
		istream >> string;

		std::cout << string << std::endl;

		if (string == "Ping") {
			response_ = "Pong\n";

			boost::asio::async_write(socket_, boost::asio::buffer(response_, response_.length()), boost::bind(&api_connection::handle_write, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
	} else {
		std::cout << "close connection..." << std::endl;

		socket_.close();
	}
}

void api_connection::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
	std::cout << "close connection..." << std::endl;
	socket_.close();
	delete this;
}