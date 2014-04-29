#include "utilities.h"

unsigned int ddsn::next_power(unsigned int number, unsigned int base) {
	unsigned int next_power = 1;

	while (next_power < number) {
		next_power *= 2;
	}

	return next_power;
}