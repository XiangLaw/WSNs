/*
 * boundhole.h
 *
 *  Created on: May 26, 2011
 *      Author: leecom
 */

#ifndef BOUNDHOLE_H_
#define BOUNDHOLE_H_

#include "wsn/geomathhelper/geo_math_helper.h"
#include "wsn/gpsr/gpsr.h"
#include "boundhole_packet.h"

#define STORAGE_ALL	0x00 // announcement
#define STORAGE_ONE 0x01 // broadcast

class BoundHoleAgent;

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

typedef void(BoundHoleAgent::*firefunction)(void);

class BoundHoleTimer : public TimerHandler
{
	public:
		BoundHoleTimer(BoundHoleAgent *a, firefunction f) : TimerHandler() {
			a_ = a;
			firing_ = f;
		}

	protected:
		virtual void expire(Event *e);
		BoundHoleAgent *a_;
		firefunction firing_;
};

class BoundHoleAgent : public GPSRAgent {
private:
	friend class BoundHoleHelloTimer;
	BoundHoleTimer findStuck_timer_;
	BoundHoleTimer boundhole_timer_;

	void startUp();

	void findStuckAngle();

	void sendBoundHole();
	void recvBoundHole(Packet*);

	void sendRefresh(Packet*);
	void recvRefresh(Packet*);

	node* getNeighborByBoundHole(Point*, Point*);

	void dumpBoundhole();

protected:
	double storage_opt_;
	double range_;
	int limit_hop; // limit_max_hop_
	int limit_min_hop_;

	stuckangle* stuck_angle_;

	polygonHole* createPolygonHole(Packet*);
	virtual void createHole(Packet*) = 0;

	virtual void sendData(Packet*) = 0;
	virtual void recvData(Packet*) = 0;

	void dumpBCEnergy();

public:
	BoundHoleAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);

};


#endif /* BOUNDHOLE_H_ */
