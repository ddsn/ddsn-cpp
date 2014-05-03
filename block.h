#ifndef DDSN_BLOCK_H
#define DDSN_BLOCK_H

#include "code.h"
#include "definitions.h"

#include <openssl/rsa.h>
#include <string>

namespace ddsn {

class block {
public:
	block();
	block(const std::string &name);
	block(const code &code);
	block(const block &block);
	~block();

	const ddsn::code &code() const;
	const BYTE *signature() const;
	const std::string &name() const;
	const BYTE *data() const;
	size_t size() const;
	RSA *owner() const;
	const BYTE *owner_hash() const;
	UINT32 occurrence() const;

	void set_code(const ddsn::code &code);
	void set_signature(const BYTE signature[256]);
	void set_name(const std::string &name);
	void set_data(const BYTE *data, size_t size);
	void set_size(size_t size);
	void set_owner(RSA *owner);
	void set_owner_hash(BYTE owner_hash[32]);
	void set_occurrence(UINT32 occurrence);

	// create code and signature from name and data
	void seal();

	// verify code/name and signature/data
	bool verify();

	int load_from_filesystem();
	int save_to_filesystem() const;
	int delete_from_filesystem() const;
private:
	ddsn::code code_;
	BYTE signature_[256];
	std::string name_;
	BYTE *data_;
	size_t size_;
	RSA *owner_;
	BYTE owner_hash_[32];
	UINT32 occurrence_;
};

}

#endif
