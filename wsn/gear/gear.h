#ifndef GEAR_H_
#define GEAR_H_

#include "config.h"
#include "agent.h"
#include "ip.h"
#include "address.h"
#include "timer-handler.h"
#include "mobilenode.h"
#include "tools/random.h"
#include "packet.h"
#include "trace.h"
#include "classifier-port.h"
#include "cmu-trace.h"

#include "gear_pkt.h"
#include "gear_neighbor.h"

class GEARAgent;

typedef void(GEARAgent::*firefunction)(void);

class GEARTimer : public TimerHandler {
public:
	GEARTimer(GEARAgent *a, firefunction f) : TimerHandler() {
		a_ = a;
		firing_ = f;
	}
protected:
	virtual void expire(Event *e);
	GEARAgent *a_;
	firefunction firing_;
};

class GEARAgent : public Agent {
private:
	friend class GEARHelloTimer;

	GEARTimer hello_timer_;

	GEARTimer update_energy_timer_;

	void getMyLocation();
	void getDestInfo();

	// functions used for receive and send packets
	void helloMsg();
	void recvHello(Packet*);
protected:
	// point to the sensor node
	MobileNode *node_;

	// for the higher layer app de-multiplexing
	PortClassifier *port_dmux_;

	nsaddr_t my_id_;
	double my_x_;
	double my_y_;

	// position of destination
	double des_x_;
	double des_y_;

	RNG randSend_;

	GNeighbors *nblist_;	// the neighbors table

	double hello_period_;
	double range_;
	double alpha_;

	double static const ENERGY_COST = 1;

	// energy-aware value
	double h_;

	Trace *tracetarget;
	void helloTimeOut();
	void updateEnergyTimeOut();
	//void traceHello();
	//void checkRNGGraph();
	//void checkGGGraph();
	//void sendRemoveRequest(nsaddr_t id);
	//void recvRmv(Packet *p);
	void startUp();
	void recvGEAR(Packet *p);
	struct gneighbor* getNeighborByGreedy(double x, double y);
	//struct neighbor* getNeighborByPerimeter(double x, double y);
	struct gneighbor* getNeighborByEnergyAware(nsaddr_t);
	void updateEnergyAware(gneighbor* next);
	void sendBack(nsaddr_t);
	void recvSendBack(Packet*);
	void dumpEnergy();
	void dumpNeighbors();
public:
	GEARAgent();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);
};

#endif /* GEAR */
