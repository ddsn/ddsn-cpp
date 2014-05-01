#ifndef DDSN_PEER_MESSAGES_H
#define DDSN_PEER_MESSAGES_H

#include "peer_connection.h"
#include "foreign_peer.h"
#include "local_peer.h"

namespace ddsn {

class foreign_peer;
class local_peer;
class peer_connection;

class peer_message {
public:
	static peer_message *create_message(local_peer &local_peer, peer_connection::pointer connection, const std::string &first_line);

	peer_message(local_peer &local_peer, peer_connection::pointer connection);
	virtual ~peer_message();

	// called immediately after receiving the first line
	virtual void first_action(int &type, size_t &expected_size) = 0;

	// provides this message with a string
	virtual void feed(const std::string &line, int &type, size_t &expected_size) = 0;

	// provides this message with a byte array
	virtual void feed(const char *data, size_t size, int &type, size_t &expected_size) = 0;

	virtual void send() = 0;
protected:
	void send(const std::string &string);
	void send(const char *bytes, size_t size);

	local_peer &local_peer_;
	peer_connection::pointer connection_;
};

class peer_hello : public peer_message {
public:
	peer_hello(local_peer &local_peer, peer_connection::pointer connection);
	peer_hello(local_peer &local_peer, peer_connection::pointer connection, std::string type);
	~peer_hello();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	int state_;
	std::string public_key_;
	std::string type_;
};

class peer_prove_identity : public peer_message {
public:
	peer_prove_identity(local_peer &local_peer, peer_connection::pointer connection);
	~peer_prove_identity();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	std::string message_;
};

class peer_verify_identity : public peer_message {
public:
	peer_verify_identity(local_peer &local_peer, peer_connection::pointer connection);
	~peer_verify_identity();

	void set_message(const std::string &message);

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	int signature_length_;
	std::string message_;
};

class peer_welcome : public peer_message {
public:
	peer_welcome(local_peer &local_peer, peer_connection::pointer connection);
	~peer_welcome();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
};

class peer_set_code : public peer_message {
public:
	peer_set_code(local_peer &local_peer, peer_connection::pointer connection);
	peer_set_code(local_peer &local_peer, peer_connection::pointer connection, code code);
	~peer_set_code();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	code code_;
};

class peer_get_code : public peer_message {
public:
	peer_get_code(local_peer &local_peer, peer_connection::pointer connection);
	peer_get_code(local_peer &local_peer, peer_connection::pointer connection, code code);
	~peer_get_code();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	code code_;
};

class peer_introduce : public peer_message {
public:
	peer_introduce(local_peer &local_peer, peer_connection::pointer connection);
	peer_introduce(local_peer &local_peer, peer_connection::pointer connection, peer_id peer_id, std::string host, int port, int layer);
	~peer_introduce();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	int state_;
	peer_id peer_id_;
	std::string host_;
	int port_;
	int layer_;
};

class peer_connect : public peer_message {
public:
	peer_connect(local_peer &local_peer, peer_connection::pointer connection);
	peer_connect(local_peer &local_peer, peer_connection::pointer connection, int layer, code code);
	~peer_connect();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	int layer_;
	code code_;
};

class peer_store_block : public peer_message {
public:
	peer_store_block(local_peer &local_peer, peer_connection::pointer connection);
	peer_store_block(local_peer &local_peer, peer_connection::pointer connection, const block *block);
	~peer_store_block();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	const block *block_;
	std::string name_;
	size_t size_;
	code code_;
};

class peer_load_block : public peer_message {
public:
	peer_load_block(local_peer &local_peer, peer_connection::pointer connection);
	peer_load_block(local_peer &local_peer, peer_connection::pointer connection, const code &code);
	~peer_load_block();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	code code_;
};

class peer_stored_block : public peer_message {
public:
	peer_stored_block(local_peer &local_peer, peer_connection::pointer connection);
	peer_stored_block(local_peer &local_peer, peer_connection::pointer connection, const code &code, std::string name, bool success);
	~peer_stored_block();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	code code_;
	std::string name_;
	bool success_;
};

class peer_deliver_block : public peer_message {
public:
	peer_deliver_block(local_peer &local_peer, peer_connection::pointer connection);
	peer_deliver_block(local_peer &local_peer, peer_connection::pointer connection, const block *block);
	~peer_deliver_block();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	const block *block_;
	size_t size_;
	code code_;
	std::string name_;
};

}

#endif