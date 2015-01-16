#ifndef __greedy_flow_2_h__
#define __greedy_flow_2_h__

#include "config.h"

/*
 * This is the flow class for source nodes to learn destination location.
 * This is because that geographical routing protocols assume
 * a source node knows the location of its destination.
 */
class GreedyFlow {
public:
	nsaddr_t dest_;		// dest addr
	float destX_;		// dest location
	float destY_;
	
	GreedyFlow(nsaddr_t, float, float);
};

#endif
