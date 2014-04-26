#include "local_peer.h"

#include <iostream>

using namespace ddsn;

local_peer::local_peer() {

}

local_peer::~local_peer() {

}

const code &local_peer::code() {
	return code_;
}

void local_peer::store(const block &block) {
	if (code_.contains(block.code())) {
		std::cout << "save " << block.code().string('_') << " to filesystem" << std::endl;
		block.save_to_filesystem();
	}
}

void local_peer::load(block &block) {
	if (code_.contains(block.code())) {
		std::cout << "load " << block.code().string('_') << " from filesystem" << std::endl;
		block.load_from_filesystem();
	}
}