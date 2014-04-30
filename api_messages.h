#ifndef DDSN_API_MESSAGES_H
#define DDSN_API_MESSAGES_H

#include "api_connection.h"
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
	virtual void feed(const char *data, size_t size, int &type, size_t &expected_size) = 0;
protected:
	local_peer &local_peer_;
	api_connection::pointer connection_;
};

class api_out_message {
public:
	api_out_message(const local_peer &local_peer);
	virtual ~api_out_message();

	// send the message
	virtual void send(api_connection::pointer connection) = 0;
protected:
	void send(api_connection::pointer connection, const std::string &string);
	void send(api_connection::pointer connection, const char *bytes, size_t size);

	const local_peer &local_peer_;
	api_connection::pointer connection_;
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
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
};

class api_in_ping : public api_in_message {
public:
	api_in_ping(local_peer &local_peer, api_connection::pointer connection);
	~api_in_ping();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
};

class api_in_store_file : public api_in_message {
public:
	api_in_store_file(local_peer &local_peer, api_connection::pointer connection);
	~api_in_store_file();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
private:
	int state_;
	std::string file_name_;
	int chunks_;
	int chunk_;
	int file_size_;
	int chunk_size_;
	int data_pointer_;
	char *data_;
};

class api_in_load_file : public api_in_message {
public:
	api_in_load_file(local_peer &local_peer, api_connection::pointer connection);
	~api_in_load_file();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
private:
	code code_;
};

class api_in_connect_peer : public api_in_message {
public:
	api_in_connect_peer(local_peer &local_peer, api_connection::pointer connection);
	~api_in_connect_peer();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
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
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
};

/*
  OUT MESSAGES
*/

class api_out_hello : public api_out_message {
public:
	api_out_hello(local_peer &local_peer);
	~api_out_hello();

	void send(api_connection::pointer connection);
};

class api_out_ping : public api_out_message {
public:
	api_out_ping(local_peer &local_peer);
	~api_out_ping();

	void send(api_connection::pointer connection);
};

class api_out_store_file : public api_out_message {
public:
	api_out_store_file(local_peer &local_peer, const code &block_code);
	~api_out_store_file();

	void send(api_connection::pointer connection);
private:
	const code &block_code_;
};

class api_out_load_file : public api_out_message {
public:
	api_out_load_file(local_peer &local_peer, const code &block_code);
	api_out_load_file(local_peer &local_peer, const std::string &file_name, size_t file_size, const code &block_code, const char *data);
	~api_out_load_file();

	void send(api_connection::pointer connection);
private:
	std::string file_name_;
	size_t file_size_;
	const code &block_code_;
	const char *data_;
};

class api_out_connect_peer : public api_out_message {
public:
	api_out_connect_peer(local_peer &local_peer);
	~api_out_connect_peer();

	void send(api_connection::pointer connection);
};

class api_out_peer_info : public api_out_message {
public:
	api_out_peer_info(local_peer &local_peer);
	~api_out_peer_info();

	void send(api_connection::pointer connection);
};

}

#endif