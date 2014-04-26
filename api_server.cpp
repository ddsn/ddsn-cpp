#include "api_server.h"
#include "definitions.h"

#include <boost/bind.hpp>

#include <iostream>

using namespace ddsn;
using boost::asio::io_service;
using boost::asio::ip::tcp;

// API CONNECTION

int api_connection::connections = 0;

api_connection::api_connection(local_peer &local_peer, io_service &io_service) : 
	local_peer_(local_peer), socket_(io_service), message_(nullptr) {
	id_ = connections++;
}

api_connection::~api_connection() {

}

tcp::socket &api_connection::socket() {
	return socket_;
}

int api_connection::id() {
	return id_;
}

void api_connection::start() {
	read_type_ == DDSN_API_MESSAGE_TYPE_STRING;

	boost::asio::async_read_until(socket_, rcv_streambuf_, "\n", boost::bind(&api_connection::handle_read, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void api_connection::send(const std::string &string) {
	std::ostream ostream(&snd_streambuf_);

	ostream << string << "\n";

	boost::asio::async_write(socket_, snd_streambuf_, boost::bind(&api_connection::handle_write, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void api_connection::send(const char *bytes, size_t size) {
	std::ostream ostream(&snd_streambuf_);

	ostream.write(bytes, size);

	boost::asio::async_write(socket_, snd_streambuf_, boost::bind(&api_connection::handle_write, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void api_connection::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred) {
	if (!error && bytes_transferred) {
		std::istream istream(&rcv_streambuf_);

		if (message_ == nullptr) {
			std::string string;
			std::getline(istream, string);

			message_ = api_message::create_message(local_peer_, *this, string);

			if (message_ == nullptr) {
				close();
				return;
			} else {
				std::cout << "API#" << id_ << " " << string << std::endl;

				message_->first_action(read_type_, read_bytes_);
			}
		} else {
			if (read_type_ == DDSN_API_MESSAGE_TYPE_BYTES) {
				char *bytes = new char[read_bytes_];
				istream.read(bytes, read_bytes_);
				message_->feed(bytes, read_bytes_, read_type_, read_bytes_);
				delete[] bytes;
			}
			else if (read_type_ == DDSN_API_MESSAGE_TYPE_STRING) {
				std::string string;
				std::getline(istream, string);
				message_->feed(string, read_type_, read_bytes_);
			}
		}

		if (read_type_ == DDSN_API_MESSAGE_TYPE_CLOSE || read_type_ == DDSN_API_MESSAGE_TYPE_ERROR) {
			delete message_;
			close();
			return;
		} else if (read_type_ == DDSN_API_MESSAGE_TYPE_END) {
			delete message_;
			message_ = nullptr;
			read_type_ = DDSN_API_MESSAGE_TYPE_STRING;
		}
		
		if (read_type_ == DDSN_API_MESSAGE_TYPE_STRING) {
			boost::asio::async_read_until(socket_, rcv_streambuf_, "\n", boost::bind(&api_connection::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
		else if (read_type_ == DDSN_API_MESSAGE_TYPE_BYTES) {
			boost::asio::async_read(socket_, rcv_streambuf_, boost::asio::transfer_exactly(read_bytes_), boost::bind(&api_connection::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
	} else {
		close();
	}
}

void api_connection::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
}

void api_connection::close() {
	std::cout << "API#" << id_ << " CLOSE (on my behalf)" << std::endl;
	socket_.close();
	delete this;
}

// API SERVER

api_server::api_server(local_peer &local_peer, io_service &io_service, int port) :
local_peer_(local_peer), io_service_(io_service), port_(port), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {

}

api_server::~api_server() {

}

void api_server::start_accept() {
	api_connection *new_connection = new api_connection(local_peer_, io_service_);

	std::cout << "Waiting for connection API#" << new_connection->id() << std::endl;

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