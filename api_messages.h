#ifndef DDSN_API_MESSAGES_H
#define DDSN_API_MESSAGES_H

#include "api_connection.h"
#include "definitions.h"
#include "local_peer.h"

#include <string>

namespace ddsn {

class api_connection;

class api_in_message {
public:
	static api_in_message *create_message(local_peer &local_peer, api_connection::pointer connection, const std::string &first_line);

	api_in_message(local_peer &local_peer, std::shared_ptr<api_connection> connection);
	virtual ~api_in_message();

	// called immediately after receiving the first line
	virtual void first_action(int &type, size_t &expected_size) = 0;

	// provides this message with a string
	virtual void feed(const std::string &line, int &type, size_t &expected_size) = 0;

	// provides this message with a byte array
	virtual void feed(const BYTE *data, size_t size, int &type, size_t &expected_size) = 0;
protected:
	local_peer &local_peer_;
	api_connection::pointer connection_;
};

class api_out_message {
public:
	api_out_message();
	virtual ~api_out_message();

	// send the message
	virtual void send(api_connection::pointer connection) = 0;
protected:
	void send(api_connection::pointer connection, const std::string &string);
	void send(api_connection::pointer connection, const BYTE *bytes, size_t size);
};

/*
  IN MESSAGES
*/

class api_in_hello : public api_in_message {
public:
	api_in_hello(local_peer &local_peer, api_connection::pointer connection);
	~api_in_hello();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const BYTE *data, size_t size, int &type, size_t &expected_size);
};

class api_in_ping : public api_in_message {
public:
	api_in_ping(local_peer &local_peer, api_connection::pointer connection);
	~api_in_ping();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const BYTE *data, size_t size, int &type, size_t &expected_size);
};

class api_in_store_file : public api_in_message {
public:
	api_in_store_file(local_peer &local_peer, api_connection::pointer connection);
	~api_in_store_file();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const BYTE *data, size_t size, int &type, size_t &expected_size);
private:
	int state_;
	std::string file_name_;
	int chunks_;
	int chunk_;
	int file_size_;
	int chunk_size_;
	int data_pointer_;
	BYTE *data_;
};

class api_in_load_file : public api_in_message {
public:
	api_in_load_file(local_peer &local_peer, api_connection::pointer connection);
	~api_in_load_file();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const BYTE *data, size_t size, int &type, size_t &expected_size);
private:
	code code_;
};

class api_in_connect_peer : public api_in_message {
public:
	api_in_connect_peer(local_peer &local_peer, api_connection::pointer connection);
	~api_in_connect_peer();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const BYTE *data, size_t size, int &type, size_t &expected_size);
private:
	std::string host_;
	int port_;
};

class api_in_peer_info : public api_in_message {
public:
	api_in_peer_info(local_peer &local_peer, api_connection::pointer connection);
	~api_in_peer_info();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const BYTE *data, size_t size, int &type, size_t &expected_size);
};

class api_in_peer_blocks : public api_in_message {
public:
	api_in_peer_blocks(local_peer &local_peer, api_connection::pointer connection);
	~api_in_peer_blocks();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const BYTE *data, size_t size, int &type, size_t &expected_size);
};

/*
  OUT MESSAGES
*/

class api_out_hello : public api_out_message {
public:
	api_out_hello();
	~api_out_hello();

	void send(api_connection::pointer connection);
};

class api_out_ping : public api_out_message {
public:
	api_out_ping();
	~api_out_ping();

	void send(api_connection::pointer connection);
};

class api_out_store_block : public api_out_message {
public:
	api_out_store_block(const block &block, bool success);
	~api_out_store_block();

	void send(api_connection::pointer connection);
private:
	const block &block_;
	bool success_;
};

class api_out_load_block : public api_out_message {
public:
	api_out_load_block(const block &block, bool success);
	~api_out_load_block();

	void send(api_connection::pointer connection);
private:
	const block &block_;
	bool success_;
};

class api_out_connect_peer : public api_out_message {
public:
	api_out_connect_peer();
	~api_out_connect_peer();

	void send(api_connection::pointer connection);
};

class api_out_peer_info : public api_out_message {
public:
	api_out_peer_info(const local_peer &local_peer);
	~api_out_peer_info();

	void send(api_connection::pointer connection);
private:
	const local_peer &local_peer_;
};

class api_out_peer_blocks : public api_out_message {
public:
	api_out_peer_blocks(const local_peer &local_peer);
	~api_out_peer_blocks();

	void send(api_connection::pointer connection);
private:
	const local_peer &local_peer_;
};

}

#endif