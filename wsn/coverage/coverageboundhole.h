//
// Created by eleven on 9/10/15.
//

#ifndef NS_CONVERAGEHOLE_H
#define NS_CONVERAGEHOLE_H

#include <wsn/runtimecounter/runtimecounter.h>
#include "config.h"

#include "agent.h"
#include "timer-handler.h"
#include "mobilenode.h"
#include "classifier-port.h"
#include "cmu-trace.h"

#include "../gpsr/gpsr.h"
#include "../boundhole/boundhole.h"
#include "coverageboundhole_packet.h"
#include "node.h"

#include "wsn/geomathhelper/geo_math_helper.h"

#define C_RED     2
#define C_WHITE   1
#define C_BLACK   0

enum DIRECTION {
    UP = -2,
    LEFT = -1,
    NONE = 0,
    RIGHT = 1,
    DOWN = 2,
};


class CoverageBoundHoleAgent;

struct sensor_neighbor : neighbor {
    Point i1_; // intersects
    Point i2_;
};

struct list {
    DIRECTION e_;
    struct list *next_;
};

struct limits {
    double min_x;
    double min_y;
    double max_x;
    double max_y;
};

typedef void(CoverageBoundHoleAgent::*fire)(void);

class CoverageBoundHoleTimer : public TimerHandler {
public:
    CoverageBoundHoleTimer(Agent *a, fire f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);

    Agent *a_;
    fire firing_;
};

class CoverageBoundHoleAgent : public GPSRAgent {
private:
    friend class BoundHoleHelloTimer;

    CoverageBoundHoleTimer boundhole_timer_;
    RunTimeCounter runTimeCounter;

    node *boundhole_node_list_; // list of node on bound hole
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
    stuckangle *cover_neighbors_; // pair of neighbors make with node to create a fragment of hole boundary

    void addSensorNeighbor(nsaddr_t, Point, int);

    sensor_neighbor *getSensorNeighbor(nsaddr_t);

    void addNeighbor(nsaddr_t, Point); // override from GPSRAgent
    node *getNextSensorNeighbor(nsaddr_t prev_node);

    void gridConstruction(polygonHole *, node *);

    bool isOutdatedCircle(node *, triangle);

    bool isSelectableTriangle(node *, node *, triangle, struct limits);

    DIRECTION nextTriangle(triangle *, node **, node **, struct limits, DIRECTION);

public:
    CoverageBoundHoleAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};

#endif //NS_CONVERAGEHOLE_H
