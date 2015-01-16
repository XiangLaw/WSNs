#ifndef _GREEDY_NBR_H__
#define _GREEDY_NBR_H__

#include "packet.h"

class GreedyNbr
{
public:
	GreedyNbr(nsaddr_t, float, float);
	nsaddr_t addr_;
	float x_;
	float y_;
};

#endif