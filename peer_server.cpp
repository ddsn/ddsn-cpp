#include "definitions.h"
#include "peer_server.h"

#include <boost/bind.hpp>

using namespace ddsn;
using boost::asio::io_service;
using boost::asio::ip::tcp;

// PEER CONNECTION

int peer_connection::connections = 0;

peer_connection::peer_connection(local_peer &local_peer, io_service &io_service) :
	local_peer_(local_peer), socket_(io_service), message_(nullptr) {
	id_ = connections++;
}

peer_connection::~peer_connection() {
	std::cout << "PEER#" << id_ << " DELETED" << std::endl;
}

tcp::socket &peer_connection::socket() {
	return socket_;
}

int peer_connection::id() {
	return id_;
}

void peer_connection::start() {
	read_type_ = DDSN_PEER_MESSAGE_TYPE_STRING;

	boost::asio::async_read_until(socket_, rcv_streambuf_, "\n", boost::bind(&peer_connection::handle_read, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void peer_connection::send(const std::string &string) {
	boost::asio::streambuf *snd_streambuf = new boost::asio::streambuf();
	std::ostream ostream(snd_streambuf);

	ostream << string;

	boost::asio::async_write(socket_, *snd_streambuf, boost::bind(&peer_connection::handle_write, shared_from_this(),
		snd_streambuf,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void peer_connection::send(const char *bytes, size_t size) {
	boost::asio::streambuf *snd_streambuf = new boost::asio::streambuf();
	std::ostream ostream(snd_streambuf);

	ostream.write(bytes, size);

	boost::asio::async_write(socket_, *snd_streambuf, boost::bind(&peer_connection::handle_write, shared_from_this(),
		snd_streambuf,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void peer_connection::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred) {
	if (!error && bytes_transferred) {
		std::istream istream(&rcv_streambuf_);

		if (message_ == nullptr) {
			std::string string;
			std::getline(istream, string);

			message_ = peer_message::create_message(local_peer_, foreign_peer_, *this, string);

			if (message_ == nullptr) {
				close();
				return;
			}
			else {
				std::cout << "PEER#" << id_ << " " << string << std::endl;

				message_->first_action(read_type_, read_bytes_);
			}
		}
		else {
			if (read_type_ == DDSN_PEER_MESSAGE_TYPE_BYTES) {
				char *bytes = new char[read_bytes_];
				istream.read(bytes, read_bytes_);
				message_->feed(bytes, read_bytes_, read_type_, read_bytes_);
				delete[] bytes;
			}
			else if (read_type_ == DDSN_PEER_MESSAGE_TYPE_STRING) {
				std::string string;
				std::getline(istream, string);
				message_->feed(string, read_type_, read_bytes_);
			}
		}

		if (read_type_ == DDSN_PEER_MESSAGE_TYPE_CLOSE || read_type_ == DDSN_PEER_MESSAGE_TYPE_ERROR) {
			delete message_;
			close();
			return;
		}
		else if (read_type_ == DDSN_PEER_MESSAGE_TYPE_END) {
			delete message_;
			message_ = nullptr;
			read_type_ = DDSN_PEER_MESSAGE_TYPE_STRING;
		}

		if (read_type_ == DDSN_PEER_MESSAGE_TYPE_STRING) {
			boost::asio::async_read_until(socket_, rcv_streambuf_, "\n", boost::bind(&peer_connection::handle_read, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
		else if (read_type_ == DDSN_PEER_MESSAGE_TYPE_BYTES) {
			boost::asio::async_read(socket_, rcv_streambuf_, boost::asio::transfer_exactly(read_bytes_), boost::bind(&peer_connection::handle_read, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
	}
	else {
		close();
	}
}

void peer_connection::handle_write(boost::asio::streambuf *snd_streambuf, const boost::system::error_code& error, std::size_t bytes_transferred) {
	delete snd_streambuf;
}

void peer_connection::close() {
	std::cout << "PEER#" << id_ << " CLOSE (on my behalf)" << std::endl;
	socket_.close();
}

// PEER SERVER

peer_server::peer_server(local_peer &local_peer, io_service &io_service, int port) :
	local_peer_(local_peer), io_service_(io_service), port_(port), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {

}

peer_server::~peer_server() {

}

void peer_server::start_accept() {
	peer_connection::pointer new_connection = peer_connection::pointer(new peer_connection(local_peer_, io_service_));

	std::cout << "Waiting for connection PEER#" << new_connection->id() << " on port " << port_ << std::endl;

	acceptor_.async_accept(new_connection->socket(),
		boost::bind(&peer_server::handle_accept, this, new_connection->shared_from_this(),
		boost::asio::placeholders::error));
}

void peer_server::handle_accept(peer_connection::pointer new_connection, const error_code& error) {
	if (!error) {
		new_connection->start();
	}

	start_accept();
}