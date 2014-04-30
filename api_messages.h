#ifndef DDSN_API_MESSAGES_H
#define DDSN_API_MESSAGES_H

#include "api_connection.h"
#include "local_peer.h"

#include <string>

namespace ddsn {

class api_connection;

class api_message {
public:
	static api_message *create_message(local_peer &local_peer, api_connection::pointer connection, const std::string &first_line);

	api_message(local_peer &local_peer, std::shared_ptr<api_connection> connection);
	virtual ~api_message();

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

class api_hello : public api_message {
public:
	api_hello(local_peer &local_peer, api_connection::pointer connection);
	~api_hello();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
};

class api_ping : public api_message {
public:
	api_ping(local_peer &local_peer, api_connection::pointer connection);
	~api_ping();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
};

class api_store_file : public api_message {
public:
	api_store_file(local_peer &local_peer, api_connection::pointer connection);
	~api_store_file();

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

class api_load_file : public api_message {
public:
	api_load_file(local_peer &local_peer, api_connection::pointer connection);
	~api_load_file();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
private:
	code code_;
};

class api_connect_peer : public api_message {
public:
	api_connect_peer(local_peer &local_peer, api_connection::pointer connection);
	~api_connect_peer();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);
private:
	std::string host_;
	int port_;
};

}

#endif