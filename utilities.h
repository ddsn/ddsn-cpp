#ifndef DDSN_UTILITIES_H
#define DDSN_UTILITIES_H

#include "definitions.h"

#include <openssl/rsa.h>
#include <string>

namespace ddsn {

UINT32 next_power(UINT32 number, UINT32 base = 2);

size_t hex_to_bytes(const std::string &hex, BYTE *bytes, size_t size);

std::string bytes_to_hex(const BYTE *bytes, size_t size);

void hash_from_rsa(RSA *public_key, BYTE hash[32]);

}

#endif