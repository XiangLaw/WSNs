/*
 * scalegoal.h
 *
 * Created on: Jan 24, 2014
 * author :    trongnguyen
 *
 * base on Goal algorithm
 * scale the convex hull by the distance of this node to hole
 * bypassing the scale hole like
 */

#ifndef SCALEGOAL_H_
#define SCALEGOAL_H_

#include "wsn/convexhull/convexhull.h"
#include "scalegoal_packet.h"

class ScaleGoalAgent : public ConvexHullAgent
{
private:
	double 	alpha_;
	int    	level_;
	double	distance_;

	void sendBCH(polygonHole* h);
	void recvBCH(Packet* p);

	//void 		 reduce(polygonHole * h);
	polygonHole* scale(polygonHole *h);
	bool 		 isInside(polygonHole*, Point*);

	double distance(polygonHole* h, Point* p);
	double maxEdge(polygonHole* h);
	int numNode(polygonHole* h);

	void dumpBroadcast();

public:
	ScaleGoalAgent();
	int command(int argc, const char*const* argv);
};

#endif
