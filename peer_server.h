#ifndef DDSN_PEER_SERVER_H
#define DDSN_PEER_SERVER_H

#include "foreign_peer.h"
#include "local_peer.h"
#include "peer_connection.h"
#include "peer_messages.h"

#include <boost/asio.hpp>

namespace ddsn {

class foreign_peer;
class peer_connection;
class peer_message;

class peer_server {
public:
	peer_server(local_peer &local_peer, boost::asio::io_service &io_service, int port = 4494);
	~peer_server();

	void set_port(int port);

	void start_accept();
	void next_accept();
private:
	void handle_accept(peer_connection::pointer new_connection, const boost::system::error_code& error);

	local_peer &local_peer_;

	boost::asio::io_service &io_service_;
	boost::asio::ip::tcp::acceptor *acceptor_;
	int port_;
};

}

#endif
