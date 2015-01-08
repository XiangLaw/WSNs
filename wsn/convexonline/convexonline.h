/*
 * convex.hdumpTime();
			dumpEnergy();
 *
 * Created on: Mar 20, 2014
 * author :    trongnguyen
 */

#ifndef CONVEXOFFLINE_H_
#define CONVEXOFFLINE_H_

#include "wsn/geomathhelper/geo_math_helper.h"
#include "wsn/gpsr/gpsr.h"
#include "wsn/gridoffline/grid_packet.h"
#include "wsn/gridonline/gridonline_packet_data.h"

class ConvexOnlineAgent;

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

typedef void(ConvexOnlineAgent::*firefunction)(void);

class ConvexOnlineTimer : public TimerHandler
{
	public:
		ConvexOnlineTimer(ConvexOnlineAgent *a, firefunction f) : TimerHandler() {
			a_ = a;
			firing_ = f;
		}

	protected:
		virtual void expire(Event *e);
		ConvexOnlineAgent *a_;
		firefunction firing_;
};

class ConvexOnlineAgent : public GPSRAgent {
private:
	friend class ConvexOnlineHelloTimer;
	ConvexOnlineTimer findStuck_timer_;
	ConvexOnlineTimer convex_timer_;

	double range_;
	double limit_;
	double r_;
	int limit_boundhole_hop_;

	stuckangle* stuck_angle_;
	polygonHole* hole_list_;

	void startUp();

	void findStuckAngle();
	node* getNeighborByBoundhole(Point*, Point*);

	void sendBoundHole();
	void recvBoundHole(Packet*);

	void addData(Packet * p);
	void addLastData(Packet * p);

	polygonHole* createPolygonHole(Packet*);
	bool reduce(polygonHole*);

	void createConvexHole(Packet*);
	void findEx(polygonHole*, node*, node*, node*);
	void addHoleNode(polygonHole*, node*);

	void dumpTime();
	void dumpBoundhole();
	void dumpArea();

public:
	ConvexOnlineAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);
};

#endif /* CONVEX_H_ */
