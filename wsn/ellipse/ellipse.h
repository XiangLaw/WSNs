/*
 * elLipse.h
 *
 * Created on: Jan 6, 2014
 * author :    trongnguyen
 */

#ifndef ELlIPSE_H_
#define ELlIPSE_H_

#include "agent.h"
#include "packet.h"

#include "ellipse_packet.h"
#include "wsn/boundhole/boundhole.h"
#include "wsn/geomathhelper/geo_math_helper.h"

class EllipseAgent;

struct ellipseHole : Ellipse
{
	int hole_id_;
	struct ellipseHole *next_;

	ellipseHole() : Ellipse()
	{
		hole_id_ = -1;
		next_ = NULL;
	}
};

class EllipseAgent : public BoundHoleAgent
{
private:
	// number set up at initialize of system
	// help calculate tangent vector
	double alpha_;

	struct ellipseHole *ellipse_list_;

	void createHole(Packet*);

	void sendBroadcast(ellipseHole*);
	void recvBroadcast(Packet*);

	void routing(Packet*);

	void sendData(Packet*);
	void recvData(Packet*);

	void dumpApproximateHole();
	void dumpBroadcast();
	void dumpRoutingTable(hdr_ellipse_data*);

public:
	EllipseAgent();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
};

#endif
