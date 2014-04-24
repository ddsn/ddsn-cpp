#ifndef DDSN_API_SERVER_H
#define DDSN_API_SERVER_H

#include "boost/asio.hpp"

#include "local_peer.h"

using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::system::error_code;

namespace ddsn {
	class api_connection : public std::enable_shared_from_this<api_connection> {
	public:
		typedef boost::shared_ptr<api_connection> pointer;

		static pointer create(boost::asio::io_service& io_service);

		tcp::socket& socket();
		void start();
	private:
		api_connection(io_service& io_service);

		tcp::socket socket_;
	};

	class api_server {
	public:
		api_server(local_peer &local_peer, io_service &io_service, int port = 4494);
		~api_server();

		void start_accept();
	private:
		void handle_accept(api_connection::pointer new_connection, const error_code& error);

		local_peer &local_peer_;
		io_service &io_service_;
		tcp::acceptor acceptor_;
		int port_;
	};
}

#endif