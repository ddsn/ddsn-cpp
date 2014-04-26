#ifndef DDSN_API_SERVER_H
#define DDSN_API_SERVER_H

#include "api_messages.h"
#include "local_peer.h"

#include <boost/asio.hpp>

using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::system::error_code;

namespace ddsn {
	class api_message;

	class api_connection : public std::enable_shared_from_this<api_connection> {
	public:
		static int connections;

		api_connection(local_peer &local_peer, io_service& io_service);
		~api_connection();

		tcp::socket& socket();
		int id();
		void start();

		void send(const std::string &string);
		void send(const char *bytes, size_t size);

		void close();
	private:
		void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
		void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

		local_peer &local_peer_;

		tcp::socket socket_;

		boost::asio::streambuf rcv_streambuf_;
		boost::asio::streambuf snd_streambuf_;
		api_message *message_;

		int read_type_;
		size_t read_bytes_;

		int id_;
	};

	class api_server {
	public:
		api_server(local_peer &local_peer, io_service &io_service, int port = 4494);
		~api_server();

		void start_accept();
	private:
		void handle_accept(api_connection *new_connection, const error_code& error);

		local_peer &local_peer_;

		io_service &io_service_;
		tcp::acceptor acceptor_;
		int port_;
	};
}

#endif
