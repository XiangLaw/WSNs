/**
 * TAAgent: base class for topological network
 * (i.e: node stores its connectivity information, not its location information)
 * TA = topological approach
 */

#ifndef TAAGENT_H_
#define TAAGENT_H_

#include "config.h"

#include "agent.h"
#include "timer-handler.h"
#include "mobilenode.h"
#include "classifier-port.h"
#include "cmu-trace.h"

#include "node.h"

#include "wsn/geomathhelper/geo_math_helper.h"

class TAAgent;
typedef struct node neighbor;

class TAAgent : public Agent {
protected:
    void startUp();						// Initialize the Agent
    double x_;
    double y_;                          // for debug only - use to construct topology
    MobileNode*		node_;				// the attached mobile node
    PortClassifier*	port_dmux_;			// for the higher layer app de-multiplexing
    Trace *			trace_target_;
    neighbor*		neighbor_list_;		// neighbor list: routing table implementation
    Point * 		dest;				// position of destination

    nsaddr_t my_id_;					// node id (address), which is essential part of topological network

    void dumpNeighbor();
    void dumpEnergy();
public:
    TAAgent();
    int  command(int, const char*const*);
    void recv(Packet*, Handler*);         //inherited virtual function
};

#endif