#pragma once

#include <wsn/runtimecounter/runtimecounter.h>
#include "config.h"

#include "agent.h"
#include "timer-handler.h"
#include "mobilenode.h"
#include "classifier-port.h"
#include "cmu-trace.h"

#include "../gpsr/gpsr.h"
#include "../boundhole/boundhole.h"
#include "hach_packet.h"
#include "node.h"

#include "wsn/geomathhelper/geo_math_helper.h"

class HACHAgent;

struct sensor_neighbor : neighbor {
    Point i1_; // intersects
    Point i2_;
};

typedef void(HACHAgent::*fire)(void);

class HACHTimer : public TimerHandler {
public:
    HACHTimer(Agent *a, fire f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);

    Agent *a_;
    fire firing_;
};

class HACHAgent : public GPSRAgent {
private:
    friend class HACHTimer;

    HACHTimer boundhole_timer_;
    RunTimeCounter runTimeCounter;

    void startUp();

    void recvCoverage(Packet *);

    bool checkBCP(node *);

    node *getBCP(Point point);

    void updateBCP(node *pNode, node *);

    node *getNextBCP(node *pNode);

    void bcpDetection();

    void holeBoundaryDetection();

    void dumpCoverageBoundhole(polygonHole *);

    void dumpPatchingHole(Point);

protected:
    double communication_range_;
    double sensor_range_;
    int limit_hop;
    int degree_coverage_;
    node *bcp_list;
    polygonHole *hole_list_;

    void addNeighbor(nsaddr_t, Point); // override from GPSRAgent
public:
    HACHAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);

    void recvHACH(Packet *p);

    void sendHACH(node, node);

    Point calculatePatchingPoint(Point a, Point b);
};
