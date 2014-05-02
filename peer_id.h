#ifndef DDSN_PEER_ID_H
#define DDSN_PEER_ID_H

#include "definitions.h"

#include <string>

namespace ddsn {

class peer_id {
public:
	peer_id();
	peer_id(BYTE id[32]);
	~peer_id();

	const BYTE *id() const;

	void set_id(BYTE id[32]);

	peer_id &operator=(const peer_id &peer_id);
	bool operator==(const peer_id &peer_id) const;

	std::string string() const;
	std::string short_string() const;

	friend std::ostream& operator<<(std::ostream& stream, const peer_id& peer_id);
	friend std::hash<ddsn::peer_id>;
private:
	BYTE id_[32];
};

}

namespace std {

template <> struct hash<ddsn::peer_id> {
	size_t operator()(const ddsn::peer_id &peer_id) const;
};

}

#endif