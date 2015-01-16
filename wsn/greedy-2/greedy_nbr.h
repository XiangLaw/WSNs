#ifndef __greedy_nbr_h__
#define __greedy_nbr_h__

#include "packet.h"

class GreedyNbr {
public:
	nsaddr_t addr_;		// The address
	float x_;			// x
	float y_;			// y
	
	GreedyNbr(nsaddr_t, float, float);
};

#endif
