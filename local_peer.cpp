#include "local_peer.h"

#include <iostream>

using namespace ddsn;
using namespace std;

local_peer::local_peer() {

}

local_peer::~local_peer() {

}

const code &local_peer::code() const {
	return code_;
}

bool local_peer::integrated() const {
	return integrated_;
}

void local_peer::store(const block &block) {
	if (code_.contains(block.code())) {
		cout << "save " << block.code().string('_') << " to filesystem" << endl;
		block.save_to_filesystem();
	}
}

void local_peer::load(block &block) {
	if (code_.contains(block.code())) {
		cout << "load " << block.code().string('_') << " from filesystem" << endl;
		block.load_from_filesystem();
	}
}