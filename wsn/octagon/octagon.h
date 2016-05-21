/*
 * octagon.h
 *
 *  Last edited on Nov 16, 2013
 *  by Trong Nguyen
 *
 * =========== BROADCAST ===========
 *
 * 1. Setup:		set storage_opt_ = BROADCAST
 *
 * 2. Routing and forward data
 *	time = 0:
 *		say Hello neighbors
 * 		BoundHole define hole, only store hole's information in control node
 *
 * 	time = 60:
 * 		Control node approximates hole to octagonHole
 * 		Control node broadcasts octagonHole's informations to all the network
 *
 * 	time = 90:
 * 		Calculate routing table
 *
 * 3. Forward data
 * 	Data packet bring routing table
 * 	in each node, try to send packet to sub-destination in routing table by greedy
 *
 * =========== ANNOUCEMENT =========
 *
 * 1. Setup:		set storage_opt_ = ANNOUCEMENT
 *
 * 2. Routing and forward data
 *	time = 0:
 *		says Hello neighbors
 * 		defines holes, sends hole's informations to all boundary-nodes of each hole
 *
 * 	time = 60:
 * 		approximates hole to octagonHole
 *
 * 	time = 90:
 * 		Calculate routing table (only for boundary nodes)
 *
 * 3. Forward data
 * 	sets type = DATA_GREEDY
 * 	tries to sends to destination by greedy
 * 	if "meet hole" <=>	node haves hole's informations && node != source && type == GREEDY
 *		sets type = DATA_ROUTING
 *		tries to sends by greedy
 *		sends an announcement packet to source node
 *
 *	receives announcement packet
 *		update routing table
 *
 */

#ifndef OCTAGON_H_
#define OCTAGON_H_

#include "agent.h"
#include "packet.h"

#include "octagon_packet.h"
#include "wsn/boundhole/boundhole.h"
#include "wsn/geomathhelper/geo_math_helper.h"

enum OCTAGON_REGION {
	REGION_1 = 1, // broadcast region
	REGION_2
};

class OctagonAgent;

struct octagonHole
{
	int hole_id_;
	double pc_; 	// perimeter of polygon
	double delta_; 	// no use???
	double d_;
	struct node* node_list_;
	struct octagonHole* next_;
};


class OctagonAgent : public BoundHoleAgent
{
private:
	AgentBroadcastTimer broadcast_timer_;

	double stretch_;
	double scale_factor_;
	double ln_;
	double alpha_;

	octagonHole* octagonHole_list_;

	OCTAGON_REGION region_;

	void dynamicRouting(Packet* p, OCTAGON_REGION region);
	void bypassingHole(octagonHole* h, Point* D, Point* routingTable, int& routingCount);

	void addrouting(Point* p, Point* routingTable, int& routingCount);
	void addHoleNode(Point newPoint);

	void createHole(Packet* p);

	void sendBroadcast(octagonHole* h);
	void recvBroadcast(Packet* p);

	void sendData(Packet* p);
	void recvData(Packet* p);

	void dumpRoutingTable();
	void dumpApproximateHole();
	void dumpDynamicRouting(Packet* p, octagonHole* hole);
	void dumpBroadcast();

public:
	OctagonAgent();
	void forwardBroadcast(Packet *p);
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
};

#endif
