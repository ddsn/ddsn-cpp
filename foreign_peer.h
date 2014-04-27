#ifndef DDSN_FOREIGN_PEER_H
#define DDSN_FOREIGN_PEER_H

#include "peer_networking/peer_connection.h"

namespace ddsn {
	class peer_connection;

	class foreign_peer {
	public:
		foreign_peer();
		~foreign_peer();
	private:
		peer_connection *peer_connection_;
	};
}

#endif