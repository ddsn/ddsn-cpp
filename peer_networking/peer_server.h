#ifndef DDSN_PEER_SERVER_H
#define DDSN_PEER_SERVER_H

#include "peer_messages.h"
#include "peer_connection.h"

#include "../local_peer.h"
#include "../foreign_peer.h"

#include <boost/asio.hpp>

using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::system::error_code;

namespace ddsn {
	class foreign_peer;
	class peer_connection;
	class peer_message;

	class peer_server {
	public:
		peer_server(local_peer &local_peer, io_service &io_service, int port = 4494);
		~peer_server();

		void start_accept();
	private:
		void handle_accept(peer_connection::pointer new_connection, const error_code& error);

		local_peer &local_peer_;

		io_service &io_service_;
		tcp::acceptor acceptor_;
		int port_;
	};
}

#endif