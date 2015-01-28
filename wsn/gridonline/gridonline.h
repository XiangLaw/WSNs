/*
 * grid.h
 *
 * Created on: Mar 20, 2014
 * author :    trongnguyen
 */

#ifndef GRIDOFFLINE_H_
#define GRIDOFFLINE_H_

#include "wsn/geomathhelper/geo_math_helper.h"
#include "wsn/gpsr/gpsr.h"
#include "wsn/gridoffline/grid_packet.h"

#include "gridonline_packet_data.h"

class GridOnlineAgent;

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

typedef void(GridOnlineAgent::*firefunction)(void);

class GridOnlineTimer : public TimerHandler
{
	public:
		GridOnlineTimer(GridOnlineAgent *a, firefunction f) : TimerHandler() {
			a_ = a;
			firing_ = f;
		}

	protected:
		virtual void expire(Event *e);
		GridOnlineAgent *a_;
		firefunction firing_;
};

class GridOnlineAgent : public GPSRAgent {
private:
	friend class GridOnlineHelloTimer;
	GridOnlineTimer findStuck_timer_;
	GridOnlineTimer grid_timer_;

	double range_;
	double limit_;
	double r_;
	int limit_boundhole_hop_;

	stuckangle* stuck_angle_;

	void startUp();

	void findStuckAngle();
	node* getNeighborByBoundhole(Point*, Point*);

	void sendBoundHole();
	void recvBoundHole(Packet*);

	void addData(Packet * p, bool isLast = false);
	void addMesh(GridOnlinePacketData *data, double x, double y, bool isLast = false);
	void reduce(GridOnlinePacketData *data);

	void createPolygonHole(Packet*);

	void dumpTime();
	void dumpBoundhole();
	void dumpArea();

protected:
	polygonHole* hole_list_;

public:
	GridOnlineAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);
};

#endif /* GRID_H_ */
