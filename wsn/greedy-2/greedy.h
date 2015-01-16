#ifndef __greedy_h__
#define __greedy_h__

#include <vector>

#include "greedy_pkt.h"
#include "greedy_nbr.h"
#include "greedy_flow.h"

#include "agent.h"
#include "packet.h"
#include "trace.h"
#include "timer-handler.h"
#include "mobilenode.h"
#include "classifier-port.h"
#include "cmu-trace.h"

class GreedyAgent;

/*
 * This is the Greedy Forwarding agent. 
 */
class GreedyAgent : public Agent {
public:
	GreedyAgent();
	int command(int, const char*const*);
	void recv(Packet*, Handler*);

protected:
	PortClassifier *port_dmux_;				// for passing packets up to agents
	Trace *logtarget_;						// for trace

	// for Greedy routing
	void forwardData(Packet*);				// forward packets
	nsaddr_t getNextNode(Packet*);			// return the next candidate node

private:
	// the node information
	MobileNode *node_;						// the attached mobile node	
	nsaddr_t addr_;							// addr
	double x_;								// the location X
	double y_;								// the location Y
	std::vector< GreedyNbr* > nbr_;			// the neighbors
	std::vector< GreedyFlow* > flow_;		// the flow

	void setLocation();								// set the location of this agent
	bool addGreedyNbr(nsaddr_t, float, float);		// add neighbors

	// for flow data
	void addFlow(nsaddr_t, float, float);		// add flow data
	GreedyFlow* getFlow(nsaddr_t);					// return flow data

	// dump
	void dumpPacket(Packet*);

};

#endif
