#ifndef DDSN_code_H
#define DDSN_code_H

#include <string>

namespace ddsn {

class code {
public:
	code();
	code(int layers);
	code(int layers, const unsigned char *code);
	code(std::string code, char delim = ':');
	code(const code &code);

	~code();

	int layer_code(int layer) const;
	int ext_layer_code(int layer) const;
	void set_layer_code(int layer, int code);

	void resize_layers(int layers);

	int layers() const;

	bool contains(const code &code) const;
	int differing_layer(const code &code) const;

	code &operator=(const code &code);
	bool operator==(const code &code) const;
	bool operator!=(const code &code) const;

	std::string string(char delim = ':') const;

	friend std::ostream& operator<<(std::ostream& stream, const code& code);
	friend std::hash<ddsn::code>;

private:
	int layers_;
	unsigned char *code_;
};

std::ostream& operator<<(std::ostream& stream, const code& code);

}

namespace std {

template <> struct hash<ddsn::code> {
	size_t operator()(const ddsn::code &code) const;
};

}

#endif