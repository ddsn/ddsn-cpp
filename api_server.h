#ifndef DDSN_API_SERVER_H
#define DDSN_API_SERVER_H

#include "api_connection.h"
#include "api_messages.h"
#include "local_peer.h"

#include <boost/asio.hpp>
#include <list>
#include <string>

using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::system::error_code;

namespace ddsn {

class api_message;

class api_server {
public:
	api_server(local_peer &local_peer, io_service &io_service, const std::string &password);
	~api_server();

	std::string password();

	void set_port(int port);

	void start_accept();
	void next_accept();

	void add_connection(api_connection::pointer connection);
	void remove_connection(api_connection::pointer connection);

	void broadcast(const std::string &string);
	void broadcost(const char *bytes, size_t size);
private:
	void handle_accept(api_connection::pointer new_connection, const error_code& error);

	local_peer &local_peer_;

	std::string password_;

	io_service &io_service_;
	tcp::acceptor *acceptor_;
	int port_;

	std::list<api_connection::pointer> connections_;
};

}

#endif
