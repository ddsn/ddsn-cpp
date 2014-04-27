#include "api_server.h"
#include "local_peer.h"
#include "peer_server.h"

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>

using namespace ddsn;
using namespace std;
namespace po = boost::program_options;

string version = "0.1";

int main(int argc, char *argv[]) {
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("version", "print version")
		("peer-port", po::value<int>()->default_value(4494), "set peer port")
		("api-port", po::value<int>()->default_value(4495), "set api port")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << "\n";
		return 1;
	}

	if (vm.count("version")) {
		cout << "DDSN version " << version << " built on " << __DATE__ << " at " << __TIME__ << "\n";
		return 1;
	}

	boost::asio::io_service io_service;

	local_peer *my_peer = new local_peer();

	peer_server peer_server(*my_peer, io_service);
	api_server api_server(*my_peer, io_service);

	if (vm.count("peer-port")) {
		peer_server.set_port(vm["peer-port"].as<int>());
	}

	if (vm.count("api-port")) {
		api_server.set_port(vm["api-port"].as<int>());
	}

	peer_server.start_accept();
	api_server.start_accept();

	io_service.run();

	return 0;
}