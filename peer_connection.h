#ifndef DDSN_PEER_CONNECTION_H
#define DDSN_PEER_CONNECTION_H

#include "local_peer.h"
#include "foreign_peer.h"

#include <boost/asio.hpp>
#include <memory>

namespace ddsn {

class foreign_peer;
class local_peer;
class peer_message;

class peer_connection : public std::enable_shared_from_this<peer_connection> {
public:
	typedef std::shared_ptr<peer_connection> pointer;

	static int connections;

	peer_connection(local_peer &local_peer, boost::asio::io_service& io_service);
	~peer_connection();

	void set_foreign_peer(foreign_peer *foreign_peer);

	boost::asio::ip::tcp::socket& socket();
	int id();
	void start();

	void send(const std::string &string);
	void send(const char *bytes, size_t size);

	void close();
private:
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
	void handle_write(boost::asio::streambuf *snd_streambuf, const boost::system::error_code& error, std::size_t bytes_transferred);

	local_peer &local_peer_;
	foreign_peer *foreign_peer_;

	boost::asio::ip::tcp::socket socket_;

	boost::asio::streambuf rcv_streambuf_;
	peer_message *message_;

	int read_type_;
	size_t read_bytes_;

	int id_;
};

}

#endif