#ifndef DDSN_PEER_CONNECTION_H
#define DDSN_PEER_CONNECTION_H

#include "definitions.h"
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

	std::shared_ptr<ddsn::foreign_peer> foreign_peer();
	bool introduced() const;
	bool got_welcome() const;

	void set_foreign_peer(std::shared_ptr<ddsn::foreign_peer> foreign_peer);
	void set_introduced(bool introduced);
	void set_got_welcome(bool got_welcome);

	boost::asio::ip::tcp::socket& socket();
	UINT32 id();
	void start();

	void close();
private:
	void send(const std::string &string);
	void send(const BYTE *bytes, size_t size);

	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
	void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

	local_peer &local_peer_;
	std::shared_ptr<ddsn::foreign_peer> foreign_peer_;

	boost::asio::ip::tcp::socket socket_;

	BYTE *rcv_buffer_;
	size_t rcv_buffer_start_;
	size_t rcv_buffer_end_;
	size_t rcv_buffer_size_;

	BYTE *snd_buffer_;
	size_t snd_buffer_start_;
	size_t snd_buffer_end_;
	size_t snd_buffer_size_;

	peer_message *message_;

	UINT32 read_type_;
	size_t read_bytes_;

	UINT32 id_;

	bool introduced_;
	bool got_welcome_;

	friend class ddsn::peer_message;
};

}

#endif
