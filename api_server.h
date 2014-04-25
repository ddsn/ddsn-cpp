#ifndef DDSN_API_SERVER_H
#define DDSN_API_SERVER_H

#include "local_peer.h"

#include <boost/asio.hpp>

using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::system::error_code;

namespace ddsn {
	class api_connection : public std::enable_shared_from_this<api_connection> {
	public:
		api_connection(io_service& io_service);

		tcp::socket& socket();
		void start();
	private:
		void handle_read_line(const boost::system::error_code& error, std::size_t bytes_transferred);
		void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

		tcp::socket socket_;

		boost::asio::streambuf streambuf_;
		std::string response_;
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
