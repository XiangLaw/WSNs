#ifndef EDGR_H_
#define EDGR_H_

#include <timer-handler.h>
#include <packet.h>
#include <wsn/geomathhelper/geo_math_helper.h>
#include <agent.h>
#include <mobilenode.h>
#include <classifier/classifier-port.h>
#include "edgr_packet.h"

class EDGRAgent;

struct neighbor: node
{
    double time_;
    float_t residual_energy_;
};

class AgentBroadcastTimer: public TimerHandler
{
public:
    AgentBroadcastTimer(EDGRAgent *a): TimerHandler()
    {
        agent_ = a;
    }

    void setParameter(Packet *p)
    {
        packet_ = p;
    }

protected:
    virtual void expire(Event *e);
    EDGRAgent *agent_;
    Packet *packet_;
};


/*
 * beacon timer which is used to fire the beacon message periodically
 */
class EDGRBeaconTimer: public TimerHandler
{
public:
    EDGRBeaconTimer(EDGRAgent *agent): TimerHandler()
    {
        agent_ = agent;
    }

protected:
    virtual void expire(Event *e);
    EDGRAgent *agent_;
};


class EDGRAgent: public Point, public Agent
{
protected:
    friend class EDGRBeaconTimer;
    EDGRBeaconTimer beacon_timer_;
    double_t hello_period_;

    neighbor* neighbor_list_;
    RNG 			randSend_;
    MobileNode*		node_;				// the attached mobile node
    PortClassifier*	port_dmux_;			// for the higher layer app de-multiplexing
    Trace *			trace_target_;
    Point* dest;

    nsaddr_t my_id_;


    void startUp();						// Initialize the Agent

    virtual void addNeighbor(nsaddr_t, Point, float_t);

    void sendBeacon();
    void recvBeacon(Packet*);
    void beacontout();					// called by timer::expire(Event*)

    void sendBurst(Packet*);
    void recvBurst(Packet*);
    void recvBurst(Packet*, hdr_burst*);

    neighbor* getNeighbor(nsaddr_t);
    neighbor* getNeighborByGreedy(Point, Point);
    neighbor* getNeighborByRightHandRule(Point);
    neighbor* getNeighborByLeftHandRule(Point);


    void dumpEnergyByTime();
    void dumpNeighbor();
    void dumpEnergy();


public:
    EDGRAgent();
    int command(int, const char*const*);
    void recv(Packet*, Handler*);

    // broadcast timer
    virtual void forwardBroadcast(Packet *p){
        hdr_cmn *cmh = HDR_CMN(p);

        cmh->direction() = hdr_cmn::DOWN;
        cmh->last_hop_ = my_id_;
        send(p, 0);
    };
};

#endif