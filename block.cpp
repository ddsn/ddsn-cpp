#include "block.h"
#include "utilities.h"

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace ddsn;
using namespace std;

block ddsn::block::copy_without_data(const block &block) {
	ddsn::block block_cp;
	
	block_cp.set_code(block.code_);
	block_cp.set_name(block.name_);
	block_cp.set_occurrence(block.occurrence_);
	block_cp.set_owner_hash(block.owner_hash_);
	block_cp.set_signature(block.signature_);
	block_cp.set_size(block.size_);

	return block_cp;
}

code ddsn::block::compute_code(const std::string name, BYTE owner_hash[32], UINT32 occurrence) {
	SHA256_CTX sha256;

	SHA256_Init(&sha256);
	SHA256_Update(&sha256, name.c_str(), name.length());
	SHA256_Update(&sha256, owner_hash, 32);
	SHA256_Update(&sha256, &occurrence, 4);

	BYTE code_bytes[32];
	SHA256_Final(code_bytes, &sha256);

	return ddsn::code(256, code_bytes);
}

code ddsn::block::compute_code(const std::string name, RSA *owner, UINT32 occurrence) {
	BYTE owner_hash[32];
	hash_from_rsa(owner, owner_hash);
	return compute_code(name, owner_hash, occurrence);
}

block::block() : data_(nullptr), size_(0), owner_(nullptr), occurrence_(0) {
}

block::block(const string &name) : name_(name), data_(nullptr), size_(0), owner_(nullptr), occurrence_(0) {
}

block::block(const ddsn::code &code) : code_(code), data_(nullptr), size_(0), owner_(nullptr), occurrence_(0) {
}

block::block(const block &block) :
code_(block.code_), name_(block.name_), size_(block.size_), owner_(block.owner_), occurrence_(block.occurrence_) {
	memcpy(signature_, block.signature_, 256);
	memcpy(owner_hash_, block.owner_hash_, 32);

	if (block.data_ != nullptr) {
		data_ = new BYTE[size_];
		memcpy(data_, block.data_, size_);
	} else {
		data_ = nullptr;
	}
}

block::~block() {
	if (data_ != nullptr) {
		delete data_;
	}
}

const code &block::code() const {
	return code_;
}

const BYTE *block::signature() const {
	return signature_;
}

const std::string &block::name() const {
	return name_;
}

const BYTE *block::data() const {
	return data_;
}

size_t block::size() const {
	return size_;
}

RSA *block::owner() const {
	return owner_;
}

const BYTE *block::owner_hash() const {
	return owner_hash_;
}

UINT32 block::occurrence() const {
	return occurrence_;
}

void block::set_code(const ddsn::code &code) {
	code_ = code;
}

void block::set_signature(const BYTE signature[256]) {
	memcpy(signature_, signature, 256);
}

void block::set_name(const std::string &name) {
	name_ = name;
}

void block::set_data(const BYTE *data, size_t size) {
	if (data_ != nullptr) {
		delete[] data_;
	}

	data_ = new BYTE[size];
	size_ = size;

	memcpy(data_, data, size);
}

void block::set_size(size_t size) {
	size_ = size;
}

void block::set_owner(RSA *owner) {
	owner_ = owner;
	hash_from_rsa(owner, owner_hash_);
}

void block::set_owner_hash(const BYTE owner_hash[32]) {
	memcpy(owner_hash_, owner_hash, 32);
}

void block::set_occurrence(UINT32 occurrence) {
	occurrence_ = occurrence;
}

void block::seal() {
	code_ = compute_code(name_, owner_, occurrence_);

	// signature

	BYTE data_hash[32];

	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, data_, size_);
	SHA256_Update(&sha256, name_.c_str(), name_.length());
	SHA256_Final(data_hash, &sha256);

	UINT32 siglen;
	RSA_sign(NID_sha256, data_hash, 32, signature_, &siglen, owner_);
}

bool block::verify() {
	code_ = compute_code(name_, owner_, occurrence_);

	// signature

	BYTE data_hash[32];

	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, data_, size_);
	SHA256_Update(&sha256, name_.c_str(), name_.length());
	SHA256_Final(data_hash, &sha256);

	if (RSA_verify(NID_sha256, data_hash, 32, signature_, 256, owner_) != 1) {
		return false;
	}

	return true;
}

int block::save_to_filesystem() const {
	if (code_.layers() != 256) {
		return -1;
	}

	ofstream file("blocks/" + code_.string('_'), ios::out | ios::binary);

	if (file.is_open()) {
		file.seekp(0, ios::beg);

		file.write((CHAR *)code_.bytes(), 32);
		file.write((CHAR *)&occurrence_, 4);
		file.write((CHAR *)signature_, 256);
		file.write(name_.c_str(), name_.length() + 1);

		// now the public key

		BIO *pub = BIO_new(BIO_s_mem());

		PEM_write_bio_RSAPublicKey(pub, owner_);

		size_t pub_len = BIO_pending(pub);

		char *pub_key = new char[pub_len];

		BIO_read(pub, pub_key, pub_len);

		BIO_free(pub);

		file.write(pub_key, pub_len);
		file.write("\n", 1);

		delete[] pub_key;

		// and the data

		file.write((CHAR *)&size_, 4);
		file.write((CHAR *)data_, size_);
		file.close();

		return 0;
	} else {
		return 1;
	}
}

int block::load_from_filesystem() {
	if (code_.layers() != 256) {
		return -1;
	}

	ifstream file("blocks/" + code_.string('_'), ios::in | ios::binary | ios::ate);

	if (file.is_open()) {
		size_t data_size = (size_t)file.tellg();

		if (data_size == 0) {
			return 3;
		}

		file.seekg(0, ios::beg);

		// code

		BYTE code_bytes[32];
		file.read((CHAR *)code_bytes, 32);

		if (ddsn::code(256, code_bytes) != code_) {
			cout << "Wrong block code in file" << endl;
			return -2;
		}

		// occurrence

		file.read((CHAR *)&occurrence_, 4);

		// signature

		file.read((CHAR *)signature_, 256);

		// name

		char c;
		name_ = "";

		while (file.good() && (c = file.get()) != '\0') {
			name_ += c;
		}

		// owner

		std::string public_key = "";
		std::string line = "";

		while (true) {
			line = "";
			while (file.good() && (c = file.get()) != '\n') {
				line += c;
			}
			if (line != "") {
				public_key += line + "\n";
			} else {
				break;
			}
		}

		BIO *pub = BIO_new(BIO_s_mem());

		BIO_write(pub, public_key.c_str(), public_key.length());

		PEM_read_bio_RSAPublicKey(pub, &owner_, NULL, NULL);

		// data size

		file.read((CHAR *)&size_, 4);

		// data

		if (data_ != nullptr) {
			delete data_;
		}

		data_ = new BYTE[size_];

		file.read((CHAR *)data_, size_);
		file.close();

		if (verify()) {
			return 0;
		} else {
			return 2;
		}
	} else {
		return 1;
	}
}

int block::delete_from_filesystem() const {
	int ret_code = std::remove(("blocks/" + code_.string('_')).c_str());
	if (ret_code == 0) {
		return 0;
	} else {
		return 1;
	}
}