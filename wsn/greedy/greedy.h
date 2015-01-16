#ifndef _GREEDY_H_
#define _GREEDY_H_ value

#include <vector>

#include "greedy_packet.h"
#include "greedy_nbr.h"

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
	PortClassifier *port_dmux_;						// for passing packets up to agents
	Trace *logtarget_;								// for trace

	// for Greedy routing
	void routing(Packet*);							// my routing implementation
	void forwardData(Packet*);						// forward packets
	nsaddr_t getNextNode(Packet*);					// return the next candidate node

private:
	void startUp();									// start simulator function?
	// the node information
	MobileNode *node_;								// the attached mobile node	
	nsaddr_t addr_;									// my addr
	double x_;										// the location X
	double y_;										// the location Y
	std::vector< GreedyNbr* > nbr_;					// the neighbors

	void setLocation();								// set the location of this agent
	bool addGreedyNbr(nsaddr_t, float, float);		// add neighbors

	// dump
	void dumpPacket(Packet*);

};

#endif
