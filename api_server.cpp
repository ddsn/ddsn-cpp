#include "api_server.h"
#include "definitions.h"

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

api_connection::api_connection(io_service &io_service) : socket_(io_service), message_(nullptr) {

}

tcp::socket &api_connection::socket() {
	return socket_;
}

void api_connection::start() {
	read_type_ == DDSN_API_MESSAGE_TYPE_STRING;

	boost::asio::async_read_until(socket_, streambuf_, "\n", boost::bind(&api_connection::handle_read, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void api_connection::send(const std::string &string) {
	// TODO: Implement
}

void api_connection::send(const char *bytes, size_t size) {
	// TODO: Implement
}

void api_connection::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred) {
	if (!error && bytes_transferred) {
		std::cout << bytes_transferred << std::endl;

		streambuf_.commit(bytes_transferred);

		if (read_type_ == DDSN_API_MESSAGE_TYPE_BYTES && streambuf_.size() < read_bytes_) {
			boost::asio::async_read(socket_, streambuf_, boost::asio::transfer_exactly(read_bytes_ - streambuf_.size()), boost::bind(&api_connection::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
			return;
		}

		std::istream istream(&streambuf_);

		if (read_type_ == DDSN_API_MESSAGE_TYPE_BYTES) {
			
		} else if (read_type_ == DDSN_API_MESSAGE_TYPE_STRING) {
		}

		if (message_ == nullptr) {
			std::string string;
			istream >> string;
			message_ = api_message::create_message(string);

			if (message_ == nullptr) {
				close();
			}
		} else {
			if (read_type_ == DDSN_API_MESSAGE_TYPE_BYTES) {
				char *bytes = new char[read_bytes_];
				istream.get(bytes, read_bytes_);
				std::cout << "received " << read_bytes_ << " bytes" << std::endl;
				message_->feed(bytes, read_bytes_, read_type_, read_bytes_);
				delete[] bytes;
			}
			else if (read_type_ == DDSN_API_MESSAGE_TYPE_STRING) {
				std::string string;
				istream >> string;
				std::cout << string << std::endl;
				message_->feed(string, read_type_, read_bytes_);
			}

			if (read_type_ == DDSN_API_MESSAGE_TYPE_CLOSE || read_type_ == DDSN_API_MESSAGE_TYPE_END) {
				close();
			} else if (read_type_ == DDSN_API_MESSAGE_TYPE_END) {
				message_ = nullptr;
				read_type_ = DDSN_API_MESSAGE_TYPE_STRING;
			}

			if (read_type_ == DDSN_API_MESSAGE_TYPE_STRING) {
				boost::asio::async_read_until(socket_, streambuf_, "\n", boost::bind(&api_connection::handle_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
			} else if (read_type_ == DDSN_API_MESSAGE_TYPE_BYTES) {
				boost::asio::async_read(socket_, streambuf_, boost::asio::transfer_exactly(read_bytes_), boost::bind(&api_connection::handle_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
			}
		}
	} else {
		close();
	}
}

void api_connection::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
	close();
}

void api_connection::close() {
	std::cout << "close connection..." << std::endl;
	socket_.close();
	delete this;
}