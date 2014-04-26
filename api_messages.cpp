#include "api_messages.h"
#include "definitions.h"

using namespace ddsn;

api_message *api_message::create_message(const std::string &first_line) {
	return nullptr;
}

api_message::api_message(local_peer &local_peer, api_connection &connection) : local_peer_(local_peer), connection_(connection) {

}

api_message::~api_message() {

}