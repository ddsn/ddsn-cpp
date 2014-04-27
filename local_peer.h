#ifndef DDSN_LOCAL_PEER_H
#define DDSN_LOCAL_PEER_H

#include "block.h"

namespace ddsn {

class local_peer {
public:
	local_peer();
	~local_peer();

	const code &code() const;
	bool integrated() const;

	void store(const block &block);
	void load(block &block);
	bool exists(const ddsn::code &code);
private:
	ddsn::code code_;
	bool integrated_;
};

}

#endif