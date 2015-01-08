/*
 * goal.h
 *
 *  Last edited on Nov 16, 2013
 *  by Trong Nguyen
 *
 * =========== algorithm ===========
 *
 * 1. Setup:		set storage_opt_ = BROADCAST
 *
 * 2. Routing and forward data
 *	time = 0:
 *		say Hello neighbors
 * 		BoundHole define polygonHole, only store polygonHole's information in control node
 *
 * 	time = 60:
 * 		Control node approximates polygonHole to convex hull
 * 		Control node broadcasts goalHole's informations by broadcast_hop_ hop
 *
 *
 * 3. Forward data
 * 	Data packet bring destination and sub-destination
 * 	in each node, try to send packet to sub-destination by greedy
 *
 *
 */

#ifndef GOAL_H_
#define GOAL_H_

#include "agent.h"
#include "packet.h"

#include "wsn/convexhull/convexhull.h"
#include "wsn/geomathhelper/geo_math_helper.h"

class GoalAgent : public ConvexHullAgent
{
private:
	int 	lastTTL_;
	double	broadcast_hop_;
	int		limit_;

	void sendBCH(polygonHole*);
	//void sendBCH(Packet*);
	void recvBCH(Packet*);

	void dumpBroadcast();

public:
	GoalAgent();
};

#endif
