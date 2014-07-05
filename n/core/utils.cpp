#include "utils.h"
#include <n/defines.h>
#include <ctime>

namespace n {
namespace core {

const void *null = 0;

uint randHelper() {
	uint bits = log2ui(RAND_MAX);
	uint num = 0;
	for(uint i = 0; i < sizeof(uint) * 8; i += bits) {
		num = (num << bits) | rand();
	}
	return num;
}

uint random(uint max, uint min) {
	static bool seed = false;
	if(!seed) {
		#ifdef N_DEBUG
		srand(0);
		#else
		time_t now = time(0);
		srand(hash(&now, sizeof(now)));
		#endif
		seed = true;
	}
	return (uint)randHelper() % (max - min) + min;
}

float random() {
	return (float)random(RAND_MAX) * 1.0f / (RAND_MAX + 1.0f);
}

float toDeg(float a) {
	return a * 180 / N_PI;
}

float toRad(float a) {
	return a / 180 * N_PI;
}

uint log2ui(uint n) {
	uint l;
	asm("bsr %1, %0\n"
	  : "=r"(l)
	  : "r" (n));
	return l;
}

uint hash(const void *key, uint len, uint seed) {
	uint m = 0x5bd1e995;
	int r = 24;
	uint h = seed ^ len;
	byte *data = (byte *)key;
	while(!(len < sizeof(uint))) {
		uint k = *(uint *)data;
		k *= m;
		k ^= k >> r;
		k *= m;
		h *= m;
		h ^= k;
		data += sizeof(uint);
		len -= sizeof(uint);
	}
	while(!(len < 4)) {
		h ^= data[len - 1] << (8 * (len - 1));
	}
	switch(len) {
		case 3:
			h ^= data[2] << 16;
		case 2:
			h ^= data[1] << 8;
		case 1:
			h ^= data[0];
			h *= m;
	};
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
}

} //core
} //n
