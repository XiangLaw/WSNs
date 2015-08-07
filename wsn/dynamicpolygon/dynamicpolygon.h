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
    AgentBroadcastTimer broadcastTimer;
	double alpha_;					// rotate angle
    int f_;

    double distanceToPolygon(node *hole);
    int vertex(node* hole) {
        int count = 0;
        for (node* tmp = hole; tmp; tmp = tmp->next_) count++;
        return count;
    }
    double longestEdge(node* hole);
    double calc_g(int n, int i, double d){
        return d/(pow(alpha_, 1.0/i) - 1)*(1/cos(2*M_PI/(n-i+1)) - 1);
    }

	void startUp();

    // override from convexhull
    void sendBCH(polygonHole *h);
    void recvBCH(Packet *p){};

    // broadcast Hole Boundary Information(HBI)
    void broadcastHBI(polygonHole *h);
    // handle HBI message
    void recvHBI(Packet *pPacket);

    polygonHole* createHoleFromPacketData(DynamicPolygonPacketData *data);
    bool simpifyPolygon(polygonHole* polygon);

	void sendData(Packet* p);
	void recvData(Packet* p);

	void dumpBoundhole();	// print boundhole
    void dumpBroadcast();

public:
	DynamicPolygonAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);

    void addData(DynamicPolygonPacketData *pData);

    void dumpRingG();
};

#endif /* DYNAMICPOLYGON_H_ */
