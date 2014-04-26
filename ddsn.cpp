#include "api_server.h"
#include "local_peer.h"

#include "boost/asio.hpp"

#include <iostream>

using namespace ddsn;

std::string version = "v0.0.1";

int main(int argc, char *argv[]) {
	std::cout << "DDSN " << version << std::endl;

	boost::asio::io_service io_service;

	local_peer *my_peer = new local_peer();

	api_server api_server(*my_peer, io_service);

	api_server.start_accept();

	io_service.run();

	return 0;
}