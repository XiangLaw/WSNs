//
// Created by eleven on 10/7/15.
//

#ifndef NS_BCPCOVERAGE_H
#define NS_BCPCOVERAGE_H

#include <wsn/runtimecounter/runtimecounter.h>
#include "config.h"

#include "agent.h"
#include "timer-handler.h"
#include "mobilenode.h"
#include "classifier-port.h"
#include "cmu-trace.h"

#include "../gpsr/gpsr.h"
#include "../boundhole/boundhole.h"
#include "bcpcoverage_packet.h"
#include "node.h"

#include "wsn/geomathhelper/geo_math_helper.h"

class BCPCoverageAgent;

struct sensor_neighbor : neighbor{
    Point i1_; // intersects
    Point i2_;
};

typedef void(BCPCoverageAgent::*fire)(void);

class BCPCoverageTimer : public TimerHandler
{
public:
    BCPCoverageTimer(Agent *a, fire f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);
    Agent *a_;
    fire firing_;
};

class BCPCoverageAgent : public GPSRAgent{
private:
    friend class BCPCoverageTimer;
    BCPCoverageTimer boundhole_timer_;
    RunTimeCounter runTimeCounter;

    void startUp();

    void recvCoverage(Packet*);

    bool checkBCP(node*);
    node *getBCP(Point point);
    void updateBCP(node *pNode, node*);
    node *getNextBCP(node *pNode);
    node *reduceBCP(node *pNode);

    void bcpDetection();
    void holeBoundaryDetection();
    bool isDuplicatePolygon(node *);

    void dumpCoverageBoundhole(polygonHole*);
    void dumpPatchingHole(Point);
protected:
    double communication_range_;
    double sensor_range_;
    int limit_hop;
    int degree_coverage_;
    node*bcp_list;
    polygonHole *hole_list_;

    void addNeighbor(nsaddr_t, Point); // override from GPSRAgent
public:
    BCPCoverageAgent();

    int  command(int, const char*const*);
    void recv(Packet*, Handler*);
};

#endif //NS_BCPCOVERAGE_H
