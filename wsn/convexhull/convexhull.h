/*
 * convexhull.h
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
 * 		Control node broadcasts convexhullHole's informations by broadcast_hop_ hop
 *
 *
 * 3. Forward data
 * 	Data packet bring destination and sub-destination
 * 	in each node, try to send packet to sub-destination by greedy
 *
 *
 */

#ifndef CONVEXHULL_H_
#define CONVEXHULL_H_

#include "agent.h"
#include "packet.h"
#include "../include/tcl.h"
#include "wsn/boundhole/boundhole_packet_data.h"
#include "wsn/boundhole/boundhole.h"
#include "wsn/geomathhelper/geo_math_helper.h"
#include "convexhull_packet.h"

class ConvexHullAgent : public BoundHoleAgent
{
private:
	void findEx(polygonHole*, node*, node*, node*);
	void addHoleNode(polygonHole*, node*);

	//Point routing(Point*);
	void routing(hdr_convexhull*);

	static Point NullPoint;

protected:

	polygonHole*  hole_list_;
	int 	limit_;

	void createHole(Packet*);
	bool reduce(polygonHole* h);

	virtual void sendBCH(polygonHole* h) = 0;		// must be implement
	//virtual void sendBCH(Packet* p) = 0;			// must be implement
	virtual void recvBCH(Packet* p) = 0;			// must be implement

	void sendData(Packet*);
	void recvData(Packet*);

	virtual void dumpBroadcast() = 0;

	void dumpApproximateHole();
	void dumpTime();
	void dumpArea(polygonHole* h, char* fileName);

public:
	ConvexHullAgent();

	int command(int, const char*const*);
	void recv(Packet*, Handler*);
};

#endif
