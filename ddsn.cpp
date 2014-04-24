#include "api_server.h"
#include "local_peer.h"

#include "boost/asio.hpp"

#include <iostream>

using namespace ddsn;

int main(int argc, char *argv[]) {
	boost::asio::io_service io_service;

	local_peer *my_peer = new local_peer();

	api_server api_server(*my_peer, io_service);

	api_server.start_accept();

	io_service.run();

	return 0;
}