#include "local_peer.h"

#include <openssl/pem.h>
#include <openssl/sha.h>
#include <fstream>
#include <iostream>

using namespace ddsn;
using namespace std;

local_peer::local_peer() : integrated_(false), blocks_(0) {

}

local_peer::~local_peer() {

}

const peer_id &local_peer::id() const {
	return id_;
}

const code &local_peer::code() const {
	return code_;
}

bool local_peer::integrated() const {
	return integrated_;
}

int local_peer::capacity() const {
	return capacity_;
}

void local_peer::set_integrated(bool integrated) {
	integrated_ = integrated;
}

void local_peer::set_capacity(int capacity) {
	capacity_ = capacity;
}

int local_peer::load_peer_key() {
	BIO *pri = BIO_new(BIO_s_mem());

	ifstream pri_file("keys/local.pem", ios::in | ios::binary | ios::ate);

	if (pri_file.is_open()) {
		size_t pri_len = pri_file.tellg();

		char *pri_key = new char[pri_len];

		pri_file.seekg(0, ios::beg);
		pri_file.read(pri_key, pri_len);
		pri_file.close();

		BIO_write(pri, pri_key, pri_len);

		PEM_read_bio_RSAPrivateKey(pri, &keypair_, NULL, NULL);

		delete[] pri_key;
	} else {
		return 1;
	}

	create_id_from_key();

	return 0;
}

int local_peer::save_peer_key() {
	BIO *pri = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPrivateKey(pri, keypair_, NULL, NULL, 0, NULL, NULL);

	size_t pri_len = BIO_pending(pri);

	char *pri_key = new char[pri_len];

	BIO_read(pri, pri_key, pri_len);

	ofstream pri_file("keys/local.pem", ios::out | ios::binary);

	if (pri_file.is_open()) {
		pri_file.write(pri_key, pri_len);
		pri_file.close();
	} else {
		return 1;
	}

	delete[] pri_key;

	return 0;
}

void local_peer::generate_peer_key() {
	keypair_ = RSA_generate_key(2048, 3, NULL, NULL);

	create_id_from_key();
}

void local_peer::load_area_keys() {

}

void local_peer::store(const block &block) {
	if (code_.contains(block.code())) {
		cout << "save " << block.code().string('_') << " to filesystem" << endl;
		block.save_to_filesystem();
	}
}

void local_peer::load(block &block) {
	if (code_.contains(block.code())) {
		cout << "load " << block.code().string('_') << " from filesystem" << endl;
		block.load_from_filesystem();
	}
}

void local_peer::create_id_from_key() {
	unsigned char *buf, *p;
	int len = i2d_RSAPublicKey(keypair_, nullptr);
	buf = new unsigned char[len];
	p = buf;
	i2d_RSAPublicKey(keypair_, &p);

	unsigned char hash[32];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, buf, len);
	SHA256_Final(hash, &sha256);

	delete[] buf;

	id_.set_id(hash);
}