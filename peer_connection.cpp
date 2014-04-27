#include "peer_connection.h"

#include "definitions.h"

#include <boost/bind.hpp>

using namespace ddsn;
using namespace std;
using boost::asio::io_service;
using boost::asio::ip::tcp;

int peer_connection::connections = 0;

peer_connection::peer_connection(local_peer &local_peer, io_service &io_service) :
local_peer_(local_peer), socket_(io_service), message_(nullptr) {
	id_ = connections++;
}

peer_connection::~peer_connection() {
	cout << "PEER#" << id_ << " DELETED" << endl;
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

void peer_connection::send(const string &string) {
	boost::asio::streambuf *snd_streambuf = new boost::asio::streambuf();
	ostream ostream(snd_streambuf);

	ostream << string;

	boost::asio::async_write(socket_, *snd_streambuf, boost::bind(&peer_connection::handle_write, shared_from_this(),
		snd_streambuf,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void peer_connection::send(const char *bytes, size_t size) {
	boost::asio::streambuf *snd_streambuf = new boost::asio::streambuf();
	ostream ostream(snd_streambuf);

	ostream.write(bytes, size);

	boost::asio::async_write(socket_, *snd_streambuf, boost::bind(&peer_connection::handle_write, shared_from_this(),
		snd_streambuf,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void peer_connection::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
	if (!error && bytes_transferred) {
		istream istream(&rcv_streambuf_);

		if (message_ == nullptr) {
			string string;
			getline(istream, string);

			message_ = peer_message::create_message(local_peer_, foreign_peer_, *this, string);

			if (message_ == nullptr) {
				close();
				return;
			}
			else {
				cout << "PEER#" << id_ << " " << string << endl;

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
				string string;
				getline(istream, string);
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

void peer_connection::handle_write(boost::asio::streambuf *snd_streambuf, const boost::system::error_code& error, size_t bytes_transferred) {
	delete snd_streambuf;
}

void peer_connection::close() {
	cout << "PEER#" << id_ << " CLOSE (on my behalf)" << endl;
	socket_.close();
}