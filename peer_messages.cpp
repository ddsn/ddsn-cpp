#include "peer_messages.h"
#include "definitions.h"

#include <boost/lexical_cast.hpp>

using namespace ddsn;
using namespace std;

peer_message *peer_message::create_message(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection, const string &first_line) {
	return nullptr;
}

peer_message::peer_message(local_peer &local_peer, foreign_peer *foreign_peer, peer_connection &connection) :
	local_peer_(local_peer), foreign_peer_(foreign_peer), connection_(connection) {

}

peer_message::~peer_message() {

}