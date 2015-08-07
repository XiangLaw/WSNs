/*
 * dynamicpolygon.h
 *
 * Created on: Mar 20, 2014
 * author :    trongnguyen
 */

#ifndef DYNAMICPOLYGON_H_
#define DYNAMICPOLYGON_H_

#include <wsn/convexhull/convexhull.h>
#include "dynamicpolygon_packet.h"

#include "wsn/geomathhelper/geo_math_helper.h"
#include "wsn/gpsr/gpsr.h"
#include "wsn/gridoffline/grid_packet.h"
#include "wsn/dynamicpolygon/dynamicpolygon_packet_data.h"

class DynamicPolygonAgent;

class DynamicPolygonAgent : public ConvexHullAgent {
private:
	friend class DynamicPolygonHelloTimer;

	int alpha_;					// rotate angle
	double range_;				// range of this node
	int limit_boundhole_hop_;	// boundhole packet will be drop after this hop

	stuckangle* stuck_angle_;
	polygonHole* hole_list_;

	Vector* vector_;

	void startUp();

	void findStuckAngle();
	node* getNeighborByBoundhole(Point*, Point*);

	void recvDynamicPolygon(Packet*);

	void sendBoundHole();
	void recvBoundHole(Packet*);

	void makeVector();

	void addData(Packet * p);
	void addLastData(Packet * p);

	polygonHole* createPolygonHole(Packet*);

    void broadcastHBI();

    void sendBCH(polygonHole *h);

    void recvBCH(Packet *p);

	void sendData(Packet* p);
	void recvData(Packet* p);

	void dumpTime();		// time to boundhole
	void dumpBoundhole();	// print boundhole
    void dumpBroadcast();

public:
	DynamicPolygonAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);

};

#endif /* DYNAMICPOLYGON_H_ */
