#include "api_server.h"
#include "local_peer.h"
#include "peer_server.h"

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <iostream>

using namespace ddsn;
using namespace std;
namespace po = boost::program_options;

string version = "0.0.1";

int main(int argc, char *argv[]) {
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("version", "print version")
		("peer-host", po::value<string>()->default_value(""), "set peer host")
		("peer-port", po::value<int>()->default_value(4494), "set peer port")
		("api-port", po::value<int>()->default_value(4495), "set api port")
		("api-password", po::value<string>()->default_value(""), "set api password")
		("integrated", "start as peer of a new network")
		("capacity", po::value<int>()->default_value(5), "maximum number of blocks to store")
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

	local_peer my_peer(io_service, vm["peer-host"].as<string>(), vm["peer-port"].as<int>());

	peer_server peer_server(my_peer, io_service);
	api_server api_server(my_peer, io_service, vm["api-password"].as<string>());

	my_peer.set_api_server(&api_server);

	peer_server.set_port(vm["peer-port"].as<int>());
	api_server.set_port(vm["api-port"].as<int>());

	if (vm.count("integrated")) {
		my_peer.set_integrated(true);
	}

	if (my_peer.load_key() != 0) {
		cout << "Generate peer key" << endl;
		my_peer.generate_key();
		my_peer.save_key();
	} else {
		cout << "Loaded peer key" << endl;
	}

	my_peer.set_capacity(vm["capacity"].as<int>());

	cout << "Your id is " << my_peer.id().short_string() << endl;

	srand((UINT32)time(nullptr));

	peer_server.start_accept();
	api_server.start_accept();

	io_service.run();

	return 0;
}