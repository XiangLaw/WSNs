//
// Created by Vu Quoc Huy  on 8/27/15.
//

#ifndef BEHDS_H
#define BEHDS_H

#include <wsn/boundhole/boundhole.h>

struct circleHole : Circle
{
	int hole_id_;
	circleHole* next_;
};

class BEHDSAgent : public BoundHoleAgent
{
private:

	circleHole* circleHole_list_;
	int 	routing_num_;
	Point	routing_table[3];

	void routing();
	void addrouting(Point* p);
	void addrouting(Point  p) { addrouting(&p); }

	void createHole(Packet* p);

	void sendHA(Packet* p, nsaddr_t saddr, Point spos);
	void recvBEHDS(Packet* p);

	void sendData(Packet* p);
	void recvData(Packet* p);
public:
	BEHDSAgent();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
};


#endif //NS_BEHDS_H
