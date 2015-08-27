/*
 * gpsr.h
 *
 * Created on: Jan 22, 2014
 * author :    trongnguyen
 */

#ifndef GPSR_ROUTING_H_
#define GPSR_ROUTING_H_

#include "config.h"

#include "agent.h"
#include "timer-handler.h"
#include "mobilenode.h"
#include "classifier-port.h"
#include "cmu-trace.h"

#include "gpsr_packet.h"
#include "node.h"

#include "wsn/geomathhelper/geo_math_helper.h"

class GPSRAgent;

struct neighbor : node
{
	double time_;
};

class AgentBroadcastTimer : public TimerHandler
{
public:
    AgentBroadcastTimer(GPSRAgent *a) : TimerHandler() {
        agent_ = a;
    }

    void setParameter(Packet *p) {
        packet_ = p;
    }

protected:
    // -------------------------------------- Timer -------------------------------------- //
    virtual void expire(Event *e);
    GPSRAgent *agent_;
    Packet *packet_;
};

/*
 * Hello timer which is used to fire the hello message periodically
 */
class GPSRHelloTimer : public TimerHandler
{
	public:
		GPSRHelloTimer(GPSRAgent* agent) : TimerHandler()
		{
			agent_ = agent;
		}

	protected:
		virtual void expire(Event *e);
		GPSRAgent* agent_;
};



class GPSRAgent : public Point, public Agent
{
protected:

	friend class GPSRHelloTimer;
	GPSRHelloTimer	hello_timer_;

	void startUp();						// Initialize the Agent

	void addNeighbor(nsaddr_t, Point);

	void sendHello();
	void recvHello(Packet*);
	void hellotout();					// called by timer::expire(Event*)

	void sendGPSR(Packet*);
	void recvGPSR(Packet*);

	void dumpNeighbor();
	void dumpEnergy(char * filename);
	void dumpEnergy();
	void dumpHopcount(Packet* p);

	double hello_period_;
	double energy_checkpoint_;

	RNG 			randSend_;
	MobileNode*		node_;				// the attached mobile node
	PortClassifier*	port_dmux_;			// for the higher layer app de-multiplexing
	Trace *			trace_target_;
	neighbor*		neighbor_list_;		// neighbor list: routing table implementation
	//double 			off_time_;			// time node go off
	Point * 		dest;				// position of destination

	nsaddr_t my_id_;					// node id (address), which is NOT necessary

	neighbor* getNeighbor(nsaddr_t);
	neighbor* getNeighborByPerimeter(Point);
	neighbor* getNeighborByGreedy(Point d, Point s);
	neighbor* getNeighborByGreedy(Point d) { return getNeighborByGreedy(d, *this); }

	void checkEnergy();

	void recvGPSR(Packet*, hdr_gpsr*);
public:

	GPSRAgent();
	int  command(int, const char*const*);
	void recv(Packet*, Handler*);         //inherited virtual function

    // broadcast timer
    virtual void forwardBroadcast(Packet *p){
        hdr_cmn *cmh = HDR_CMN(p);

        cmh->direction() = hdr_cmn::DOWN;
        cmh->last_hop_ = my_id_;
        send(p, 0);
    };
};

#endif
