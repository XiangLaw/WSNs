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

#define C_GRAY    4 // inside hole cell
#define C_BLUE    3 // init color
#define C_RED     2 // painted cell
#define C_BLACK   1 // boundary cell
#define C_WHITE   0 // outside hole cell

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

struct direction_list {
    DIRECTION e_;
    struct direction_list *next_;
};

struct removable_cell_list {
    Point intersection;
    Point triangle;
    struct removable_cell_list *next;
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

    void gridConstruction(polygonHole *, node *, node *);

    bool isOutdatedCircle(node *, triangle);

    bool isSelectableTriangle(node *, node *, triangle, removable_cell_list **);

    DIRECTION nextTriangle(triangle *, node **, node **, DIRECTION, removable_cell_list **);

public:
    CoverageBoundHoleAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);

    void dumpCoverageGrid(triangle);

    void patchingHole(removable_cell_list *, double, double, int8_t **, int, int);

    int black_node_count(int8_t **, int, int);

    void dumpPatchingHole(Point);

    void fillGrid(int8_t **grid, int nx, int ny);
};

#endif //NS_CONVERAGEHOLE_H
