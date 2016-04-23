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

    bool isBoundary;

    void startUp();

    bool boundaryNodeDetection();

    void holeBoundaryDetection();

    void recvCoverage(Packet *);

protected:
    double communication_range_;
    double sensor_range_;
    int limit_hop_;
    sensor_neighbor *sensor_neighbor_list_; // list of the neighbors by sensor range, fixed: sensor range = 1/2*communication range
    polygonHole *hole_list_;
    polygonHole *boundhole_node_list_; // list of node on bound hole
    stuckangle *cover_neighbors_; // pair of neighbors make with node to create a fragment of hole boundary

    void addSensorNeighbor(nsaddr_t, Point, int);

    sensor_neighbor *getSensorNeighbor(nsaddr_t);

    void addNeighbor(nsaddr_t, Point); // override from GPSRAgent
    node *getNextSensorNeighbor(nsaddr_t prev_node);

    void dumpPatchingHole(Point);

    void recvHACH(Packet *p);

    void sendHACH(node, node);

    Point calculatePatchingPoint(Point a, Point b);

public:
    HACHAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};
