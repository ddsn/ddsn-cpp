#ifndef DDSN_PEER_ID_H
#define DDSN_PEER_ID_H

#include <string>

namespace ddsn {

class peer_id {
public:
	peer_id();
	peer_id(unsigned char id[32]);
	~peer_id();

	const unsigned char *id() const;

	void set_id(unsigned char id[32]);

	peer_id &operator=(const peer_id &peer_id);

	std::string string() const;

	friend std::ostream& operator<<(std::ostream& stream, const peer_id& peer_id);

private:
	unsigned char id_[32];
};

}

#endif