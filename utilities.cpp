#include "utilities.h"

#include <openssl/sha.h>
#include <cassert>

UINT32 ddsn::next_power(UINT32 number, UINT32 base) {
	UINT32 next_power = 1;

	while (next_power < number) {
		next_power *= 2;
	}

	return next_power;
}

size_t ddsn::hex_to_bytes(const std::string &hex, BYTE *bytes, size_t size) {
	assert(hex.length() % 2 == 0);
	if (hex.length() > 2 * size) {
		size = hex.length() / 2;
	}
	for (UINT32 i = 0; i < size; i++) {
		UINT32 hi, lo;
		if (hex[2 * i] >= '0' && hex[2 * i] <= '9') {
			hi = hex[2 * i] - '0';
		} else if (hex[2 * i] >= 'a' && hex[2 * i] <= 'f') {
			hi = hex[2 * i] - 'a' + 10;
		}
		if (hex[2 * i + 1] >= '0' && hex[2 * i + 1] <= '9') {
			lo = hex[2 * i + 1] - '0';
		}
		else if (hex[2 * i + 1] >= 'a' && hex[2 * i + 1] <= 'f') {
			lo = hex[2 * i + 1] - 'a' + 10;
		}
		bytes[i] = (hi << 4) | lo;
	}
	return size;
}

std::string ddsn::bytes_to_hex(const BYTE *bytes, size_t size) {
	std::string string = "";
	for (UINT32 i = 0; i < size; i++) {
		UINT32 digit = bytes[i] >> 4;
		if (digit <= 9) {
			string += (char)('0' + digit);
		} else {
			string += (char)('a' + digit - 10);
		}
		digit = bytes[i] & 0xF;
		if (digit <= 9) {
			string += (char)('0' + digit);
		}
		else {
			string += (char)('a' + digit - 10);
		}
	}
	return string;
}

void ddsn::hash_from_rsa(RSA *public_key, BYTE hash[32]) {
	BYTE *buf, *p;
	int len = i2d_RSAPublicKey(public_key, nullptr);
	buf = new BYTE[len];
	p = buf;
	i2d_RSAPublicKey(public_key, &p);

	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, buf, len);
	SHA256_Final(hash, &sha256);

	delete[] buf;
}