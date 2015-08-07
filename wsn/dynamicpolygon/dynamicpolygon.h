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

struct stuckangle
{
	// two neighbors that create stuck angle with node.
	node *a_;
	node *b_;

	stuckangle *next_;
};

struct polygonHole
{
	int hole_id_;
	struct node* node_list_;
	struct polygonHole* next_;

	~polygonHole()
	{
		node* temp = node_list_;
		do {
			delete temp;
			temp = temp->next_;
		} while(temp && temp != node_list_);
	}

	void circleNodeList()
	{
		node* temp = node_list_;
		while (temp->next_ && temp->next_ != node_list_) temp = temp->next_;
		temp->next_ = node_list_;
	}
};

typedef void(DynamicPolygonAgent::*firefunction)(void);

class DynamicPolygonTimer : public TimerHandler
{
	public:
		DynamicPolygonTimer(DynamicPolygonAgent *a, firefunction f) : TimerHandler() {
			a_ = a;
			firing_ = f;
		}

	protected:
		virtual void expire(Event *e);
		DynamicPolygonAgent *a_;
		firefunction firing_;
};

class DynamicPolygonAgent : public ConvexHullAgent {
private:
	friend class DynamicPolygonHelloTimer;
	DynamicPolygonTimer findStuck_timer_;
	DynamicPolygonTimer boundhole_timer_;

	int vertex_num_;			// number of vertexes in polygon hole
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

	void sendData(Packet* p);
	void recvData(Packet* p);

	void dumpTime();		// time to boundhole
	void dumpBoundhole();	// print boundhole

public:
	DynamicPolygonAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);

    void broadcastHBI();

    void sendBCH(polygonHole *h);
};

#endif /* DYNAMICPOLYGON_H_ */
