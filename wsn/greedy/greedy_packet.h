#ifndef _GREEDY_PACKET_H__
#define _GREEDY_PACKET_H__
#include "packet.h"

struct hdr_greedy {
	nsaddr_t src_;				// the src location
	nsaddr_t dest_;				// the dest location
	float destX_;				// x-offset of dest location
	float destY_; 				// y-offset of dest location

	inline int size() {
		unsigned int s = 2 * sizeof(nsaddr_t) + 2 * sizeof(float);
		return s;
	}
};

#endif /* _GREEDY_PACKET_H__ */