#ifndef DDSN_LOCAL_PEER_H
#define DDSN_LOCAL_PEER_H

#include "code.h"

namespace ddsn {
	class local_peer {
	public:
		local_peer();
		~local_peer();

	private:
		code code_;
	};
}

#endif