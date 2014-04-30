#ifndef DDSN_API_CONNECTION_H
#define DDSN_API_CONNECTION_H

#include "local_peer.h"

#include <boost/asio.hpp>

using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::system::error_code;

namespace ddsn {

class api_in_message;
class api_server;
class api_out_message;

class api_connection : public std::enable_shared_from_this<api_connection> {
public:
	typedef std::shared_ptr<api_connection> pointer;

	static int connections;

	api_connection(api_server &server, local_peer &local_peer, io_service& io_service);
	~api_connection();

	tcp::socket &socket();
	api_server &server();
	int id();
	bool authenticated();
	void set_authenticated(bool authenticated);

	void start();

	void close();
private:
	void send(const std::string &string);
	void send(const char *bytes, size_t size);

	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
	void handle_write(boost::asio::streambuf *snd_streambuf, const boost::system::error_code& error, std::size_t bytes_transferred);

	api_server &server_;
	local_peer &local_peer_;

	tcp::socket socket_;

	bool authenticated_;

	boost::asio::streambuf rcv_streambuf_;
	char *rcv_buffer_;
	size_t rcv_buffer_start_;
	size_t rcv_buffer_end_;
	size_t rcv_buffer_size_;
	api_in_message *message_;

	int read_type_;
	size_t read_bytes_;

	int id_;

	friend class ddsn::api_out_message;
};

}

#endif
