#include "peer_connection.h"

#include "peer_messages.h"
#include "definitions.h"
#include "utilities.h"

#include <boost/bind.hpp>

using namespace ddsn;
using namespace std;
using boost::asio::io_service;
using boost::asio::ip::tcp;

int peer_connection::connections = 0;

peer_connection::peer_connection(local_peer &local_peer, io_service &io_service) :
local_peer_(local_peer), socket_(io_service), message_(nullptr), introduced_(false), rcv_buffer_start_(0), rcv_buffer_end_(0) {
	id_ = connections++;
	rcv_buffer_ = new char[256];
	rcv_buffer_size_ = 256;
}

peer_connection::~peer_connection() {
	cout << "PEER#" << id_ << " DELETED" << endl;
	delete[] rcv_buffer_;
}

bool peer_connection::introduced() const {
	return introduced_;
}

bool peer_connection::got_welcome() const {
	return got_welcome_;
}

std::shared_ptr<foreign_peer> peer_connection::foreign_peer() {
	return foreign_peer_;
}

void peer_connection::set_foreign_peer(std::shared_ptr<ddsn::foreign_peer> foreign_peer) {
	foreign_peer_ = foreign_peer;
	foreign_peer->set_connection(shared_from_this());
}

void peer_connection::set_introduced(bool introduced) {
	introduced_ = introduced;
}

void peer_connection::set_got_welcome(bool got_welcome) {
	got_welcome_ = got_welcome;
}

tcp::socket &peer_connection::socket() {
	return socket_;
}

int peer_connection::id() {
	return id_;
}

void peer_connection::start() {
	read_type_ = DDSN_MESSAGE_TYPE_STRING;

	socket_.async_read_some(boost::asio::buffer(rcv_buffer_, rcv_buffer_size_), boost::bind(&peer_connection::handle_read, shared_from_this(),
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

	boost::asio::async_write(socket_, *snd_streambuf, boost::asio::transfer_exactly(size), boost::bind(&peer_connection::handle_write, shared_from_this(),
		snd_streambuf,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void peer_connection::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
	if (error) {
		cout << "An error occurred: " << error.message() << endl;

		close();
	}

	if (bytes_transferred) {
		rcv_buffer_end_ += bytes_transferred;

		bool comsumed;
		size_t buffer_data;

		do {
			comsumed = false;
			buffer_data = rcv_buffer_end_ - rcv_buffer_start_;

			if (message_ == nullptr || read_type_ == DDSN_MESSAGE_TYPE_STRING || read_type_ == DDSN_MESSAGE_TYPE_END) {
				int end_line = -1;

				for (unsigned int i = rcv_buffer_start_; i < rcv_buffer_end_; i++) {
					if (rcv_buffer_[i] == '\n') {
						end_line = i;
						break;
					}
				}

				if (end_line != -1) {
					std::string line(rcv_buffer_ + rcv_buffer_start_, end_line - rcv_buffer_start_);
					rcv_buffer_start_ = end_line + 1;

					if (message_ == nullptr) {
						message_ = peer_message::create_message(local_peer_, shared_from_this(), line);

						if (message_ == nullptr) {
							close();
							return;
						} else {
							cout << "PEER#" << id_ << " message: " << line << endl;

							message_->first_action(read_type_, read_bytes_);
						}
					} else {
						message_->feed(line, read_type_, read_bytes_);
					}

					comsumed = true;
				}
			} else {
				if (read_bytes_ <= buffer_data) {
					int tmp = read_bytes_;
					message_->feed(rcv_buffer_ + rcv_buffer_start_, read_bytes_, read_type_, read_bytes_);
					rcv_buffer_start_ += tmp;

					comsumed = true;
				}
			}

			if (comsumed) {
				if (read_type_ == DDSN_MESSAGE_TYPE_CLOSE || read_type_ == DDSN_MESSAGE_TYPE_ERROR) {
					delete message_;
					close();
					return;
				} else if (read_type_ == DDSN_MESSAGE_TYPE_END) {
					delete message_;
					message_ = nullptr;
					read_type_ = DDSN_MESSAGE_TYPE_STRING;
				}
			}

		} while (comsumed);

		if (read_type_ == DDSN_MESSAGE_TYPE_BYTES && read_bytes_ > DDSN_MESSAGE_CHUNK_MAX_SIZE) {
			delete message_;
			close();
			return;
		}

		size_t buffer_size = rcv_buffer_size_ - rcv_buffer_end_;

		if (buffer_size < 16 || (read_type_ == DDSN_MESSAGE_TYPE_BYTES && read_bytes_ > buffer_size)) {
			if (read_type_ == DDSN_MESSAGE_TYPE_STRING && rcv_buffer_start_ == 0 && buffer_size == 0) {
				// double buffer space because string seems to be too long for current buffer
				char *new_buffer = new char[rcv_buffer_size_ * 2];
				memcpy(new_buffer, rcv_buffer_, rcv_buffer_size_);
				delete[] rcv_buffer_;
				rcv_buffer_ = new_buffer;

				buffer_size = rcv_buffer_size_;
				rcv_buffer_size_ = rcv_buffer_size_ * 2;

				if (rcv_buffer_size_ > DDSN_MESSAGE_STRING_MAX_LENGTH) {
					// string simply too long
					close();
					return;
				}
			} else if (read_type_ == DDSN_MESSAGE_TYPE_STRING || read_bytes_ <= rcv_buffer_size_) {
				// shift to beginning to create space at the end (doesn't resize the buffer)
				memmove(rcv_buffer_, rcv_buffer_ + rcv_buffer_start_, buffer_data);

				rcv_buffer_start_ = 0;
				rcv_buffer_end_ = buffer_data;
				buffer_size = rcv_buffer_size_ - buffer_data;
			} else {
				// enlarge buffer space for expected bytes to next power of 2
				rcv_buffer_size_ = next_power(read_bytes_);
				char *new_buffer = new char[rcv_buffer_size_];
				memcpy(new_buffer, rcv_buffer_ + rcv_buffer_start_, buffer_data);
				delete[] rcv_buffer_;
				rcv_buffer_ = new_buffer;

				rcv_buffer_start_ = 0;
				rcv_buffer_end_ = buffer_data;
				buffer_size = rcv_buffer_size_ - buffer_data;
			}
		}

		socket_.async_read_some(boost::asio::buffer(rcv_buffer_ + rcv_buffer_end_, buffer_size), boost::bind(&peer_connection::handle_read, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
}

void peer_connection::handle_write(boost::asio::streambuf *snd_streambuf, const boost::system::error_code& error, size_t bytes_transferred) {
	delete snd_streambuf;
}

void peer_connection::close() {
	foreign_peer_->set_connection(nullptr);
	cout << "PEER#" << id_ << " CLOSE" << endl;
	socket_.close();
}
