#ifndef DDSN_code_H
#define DDSN_code_H

#include "definitions.h"

#include <string>

namespace ddsn {

class code {
public:
	code();
	code(int layers);
	code(int layers, const BYTE *code);
	code(std::string code, CHAR delim = ':');
	code(const code &code);

	~code();

	int layer_code(UINT32 layer) const;
	int ext_layer_code(UINT32 layer) const;
	void set_layer_code(UINT32 layer, UINT32 code);

	void resize_layers(UINT32 layers);

	UINT32 layers() const;

	bool contains(const code &code) const;
	int differing_layer(const code &code) const;

	const BYTE *bytes() const;

	code &operator=(const code &code);
	bool operator==(const code &code) const;
	bool operator!=(const code &code) const;

	std::string string(char delim = ':') const;

	friend std::ostream& operator<<(std::ostream& stream, const code& code);
	friend std::hash<ddsn::code>;

private:
	UINT32 layers_;
	BYTE *code_;
};

std::ostream& operator<<(std::ostream& stream, const code& code);

}

namespace std {

template <> struct hash<ddsn::code> {
	size_t operator()(const ddsn::code &code) const;
};

}

#endif