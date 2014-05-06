#include "api_connection.h"

#include "api_messages.h"
#include "api_server.h"
#include "definitions.h"
#include "utilities.h"

#include <boost/bind.hpp>
#include <iostream>

using namespace ddsn;
using namespace std;
using boost::asio::io_service;
using boost::asio::ip::tcp;

// API CONNECTION

int api_connection::connections = 0;

api_connection::api_connection(api_server &api_server, local_peer &local_peer, io_service &io_service) :
server_(api_server), local_peer_(local_peer), socket_(io_service), authenticated_(false), message_(nullptr), rcv_buffer_start_(0), rcv_buffer_end_(0) {
	id_ = connections++;

	rcv_buffer_ = new BYTE[256];
	rcv_buffer_size_ = 256;
}

api_connection::~api_connection() {
	std::cout << "API#" << id_ << " DELETED" << std::endl;
	delete[] rcv_buffer_;
}

tcp::socket &api_connection::socket() {
	return socket_;
}

api_server &api_connection::server() {
	return server_;
}

int api_connection::id() {
	return id_;
}

bool api_connection::authenticated() {
	return authenticated_;
}

void api_connection::set_authenticated(bool authenticated) {
	authenticated_ = authenticated;
}

void api_connection::start() {
	read_type_ = DDSN_MESSAGE_TYPE_STRING;

	socket_.async_read_some(boost::asio::buffer(rcv_buffer_, rcv_buffer_size_), boost::bind(&api_connection::handle_read, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void api_connection::send(const string &string) {
	send((BYTE *)string.c_str(), string.length());
}

void api_connection::send(const BYTE *bytes, size_t size) {
	// data still to be sent
	size_t buffer_data = snd_buffer_end_ - snd_buffer_start_;

	// space at the end of the buffer
	size_t buffer_space = snd_buffer_size_ - snd_buffer_end_;

	if (buffer_data == 0) {
		snd_buffer_start_ = 0;
		snd_buffer_end_ = 0;
	}

	if (size > buffer_space) {
		if (size <= snd_buffer_size_ - buffer_data) {
			// shift to beginning to create space at the end (doesn't resize the buffer)
			memmove(snd_buffer_, snd_buffer_ + snd_buffer_start_, buffer_data);

			snd_buffer_start_ = 0;
			snd_buffer_end_ = buffer_data;
			//new_buffer_space = snd_buffer_size_ - buffer_data;
		}
		else {
			// enlarge buffer space for expected bytes to next power of 2
			snd_buffer_size_ = next_power(buffer_data + size);
			BYTE *new_buffer = new BYTE[snd_buffer_size_];
			memcpy(new_buffer, snd_buffer_ + snd_buffer_start_, buffer_data);
			delete[] snd_buffer_;
			snd_buffer_ = new_buffer;

			snd_buffer_start_ = 0;
			snd_buffer_end_ = buffer_data;
			//new_buffer_space = snd_buffer_size_ - buffer_data;
		}
	}

	// copy the data to the buffer
	memcpy(snd_buffer_ + snd_buffer_end_, bytes, size);

	snd_buffer_end_ += size;


	if (buffer_data == 0) {
		// only send when there's not already a send request in the queue
		// if buffer_data != 0 handle_write will call async_write_some
		socket_.async_write_some(boost::asio::buffer(snd_buffer_ + snd_buffer_start_, buffer_data + size), boost::bind(&api_connection::handle_write, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
}

void api_connection::handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
	if (error) {
		cout << "An error occurred: " << error.message() << endl;
		close();
	}

	if (bytes_transferred) {
		snd_buffer_start_ += bytes_transferred;

		size_t buffer_data = snd_buffer_end_ - snd_buffer_start_;

		if (buffer_data != 0) {
			socket_.async_write_some(boost::asio::buffer(snd_buffer_ + snd_buffer_start_, buffer_data), boost::bind(&api_connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}
	}
	else {
		cout << "An error occurred: no bytes transferred" << endl;
		close();
	}
}

void api_connection::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
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

				for (UINT32 i = rcv_buffer_start_; i < rcv_buffer_end_; i++) {
					if (rcv_buffer_[i] == '\n') {
						end_line = i;
						break;
					}
				}

				if (end_line != -1) {
					std::string line((CHAR *)(rcv_buffer_ + rcv_buffer_start_), end_line - rcv_buffer_start_);
					rcv_buffer_start_ = end_line + 1;

					if (message_ == nullptr) {
						message_ = api_in_message::create_message(local_peer_, shared_from_this(), line);

						if (message_ == nullptr) {
							close();
							return;
						}
						else {
							cout << "API#" << id_ << " message: " << line << endl;

							message_->first_action(read_type_, read_bytes_);
						}
					}
					else {
						message_->feed(line, read_type_, read_bytes_);
					}

					comsumed = true;
				}
			}
			else {
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
				}
				else if (read_type_ == DDSN_MESSAGE_TYPE_END) {
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

		// if there's nothing left in the buffer to be processes, we can start using the buffer from the beginning
		if (buffer_data == 0) {
			rcv_buffer_start_ = 0;
			rcv_buffer_end_ = 0;
		}

		size_t buffer_space = rcv_buffer_size_ - rcv_buffer_end_;
		size_t bytes_to_read = read_bytes_ - buffer_data;

		if (buffer_space < 16 || (read_type_ == DDSN_MESSAGE_TYPE_BYTES && bytes_to_read > buffer_space)) {
			// we deem the buffer too small
			if (read_type_ == DDSN_MESSAGE_TYPE_STRING && rcv_buffer_start_ == 0 && buffer_space == 0) {
				// double buffer space because string seems to be too long for current buffer
				BYTE *new_buffer = new BYTE[rcv_buffer_size_ * 2];
				memcpy(new_buffer, rcv_buffer_, rcv_buffer_size_);
				delete[] rcv_buffer_;
				rcv_buffer_ = new_buffer;

				buffer_space = rcv_buffer_size_;
				rcv_buffer_size_ = rcv_buffer_size_ * 2;

				if (rcv_buffer_size_ > DDSN_MESSAGE_STRING_MAX_LENGTH) {
					// string simply too long
					close();
					return;
				}
			}
			else if (read_type_ == DDSN_MESSAGE_TYPE_STRING || bytes_to_read <= rcv_buffer_size_) {
				// shift to beginning to create space at the end (doesn't resize the buffer)
				memmove(rcv_buffer_, rcv_buffer_ + rcv_buffer_start_, buffer_data);

				rcv_buffer_start_ = 0;
				rcv_buffer_end_ = buffer_data;
				buffer_space = rcv_buffer_size_ - buffer_data;
			}
			else {
				// enlarge buffer space for expected bytes to next power of 2
				rcv_buffer_size_ = next_power(read_bytes_);
				BYTE *new_buffer = new BYTE[rcv_buffer_size_];
				memcpy(new_buffer, rcv_buffer_ + rcv_buffer_start_, buffer_data);
				delete[] rcv_buffer_;
				rcv_buffer_ = new_buffer;

				rcv_buffer_start_ = 0;
				rcv_buffer_end_ = buffer_data;
				buffer_space = rcv_buffer_size_ - buffer_data;
			}
		}

		socket_.async_read_some(boost::asio::buffer(rcv_buffer_ + rcv_buffer_end_, buffer_space), boost::bind(&api_connection::handle_read, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
	else {
		cout << "An error occurred: no bytes transferred" << endl;
		close();
	}
}

void api_connection::close() {
	std::cout << "API#" << id_ << " CLOSE (on my behalf)" << std::endl;
	server_.remove_connection(shared_from_this());
	socket_.close();
}
