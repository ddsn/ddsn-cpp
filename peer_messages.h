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
	static peer_message *create_message(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection, const std::string &first_line);

	peer_message(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection);
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

	std::shared_ptr<foreign_peer> foreign_peer_;
	local_peer &local_peer_;
	peer_connection::pointer connection_;
};

class peer_hello : public peer_message {
public:
	peer_hello(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection);
	~peer_hello();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
private:
	int state_;
	std::string public_key_;
};

class peer_prove_identity : public peer_message {
public:
	peer_prove_identity(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection);
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
	peer_verify_identity(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection);
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
	peer_welcome(local_peer &local_peer, std::shared_ptr<foreign_peer> foreign_peer, peer_connection::pointer connection);
	~peer_welcome();

	void first_action(int &type, size_t &expected_size);
	void feed(const std::string &line, int &type, size_t &expected_size);
	void feed(const char *data, size_t size, int &type, size_t &expected_size);

	void send();
};

}

#endif