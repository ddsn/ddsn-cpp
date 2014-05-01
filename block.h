#ifndef DDSN_BLOCK_H
#define DDSN_BLOCK_H

#include "code.h"

#include <string>

namespace ddsn {

class block {
public:
	block(const std::string &name);
	block(const code &code);
	~block();

	void set_data(const char *data, size_t size);
	const char *data() const;
	const ddsn::code &code() const;
	const std::string &name() const;
	size_t size() const;

	int load_from_filesystem();
	int save_to_filesystem() const;
private:
	std::string name_;
	ddsn::code code_;
	char *data_;
	size_t size_;
};

}

#endif
