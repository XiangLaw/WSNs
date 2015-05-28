/*
 * hexagon.h
 *
 *  Last edited on Nov 16, 2013
 *  author Trong Nguyen
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
 * 		Control node approximates hole to circleHole
 * 		Control node broadcasts circleHole's informations to all the network
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
 * 		approximates hole to circleHole
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

#ifndef HEXAGON_H_
#define HEXAGON_H_

#include "agent.h"
#include "packet.h"

#include "hexagon_packet.h"
#include "wsn/boundhole/boundhole.h"
#include "wsn/geomathhelper/geo_math_helper.h"

struct circleHole : Circle
{
	int hole_id_;
	circleHole* next_;
};

class HexagonAgent : public BoundHoleAgent
{
private:

	circleHole* circleHole_list_;
	int 	routing_num_;
	Point	routing_table[4];

	void routing();
	void addrouting(Point* p);

	void createHole(Packet* p);

	void sendHexagon(nsaddr_t saddr, Point spos);
	void recvHexagon(Packet* p);

	void sendData(Packet* p);
	void recvData(Packet* p);

	void dumpRoutingTable();
	void dumpApproximateHole();

public:
	HexagonAgent();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
};

#endif
