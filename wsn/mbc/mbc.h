/**
 * MBC - MaxBound Coverage algorithm
 * A Simple Method for the Deployment of Wireless Sensors to Ensure Full Coverage of an Irregular Area with Obstacles
 * This implementation implements only algorithm described in section 5 to compare with our coverage algorithm.
*/

#ifndef MBC_H
#define MBC_H

#include <wsn/gpsr/gpsr.h>
#include <wsn/runtimecounter/runtimecounter.h>
#include <wsn/boundhole/boundhole.h>

class MbcAgent;

struct sensor_neighbor : neighbor {
    Point i1_; // intersects
    Point i2_;
};

struct custom_node : Point {
    custom_node *next_;
    bool is_removable_;
};

typedef void(MbcAgent::*fire)(void);

class MbcTimer : public TimerHandler {
public:
    MbcTimer(Agent *a, fire f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);

    Agent *a_;
    fire firing_;
};

class MbcAgent : public GPSRAgent {
private:
    friend class BoundHoleHelloTimer;

    MbcTimer boundhole_timer_;
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

    void addNodeToList(double, double, custom_node **);

    void removeNodeFromList(custom_node *, custom_node **);

    void eliminateNode(custom_node *, custom_node **, polygonHole *);

public:
    MbcAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);

    void patchingHole(polygonHole *);

    void dumpPatchingHole(Point);

    void dumpPatchingHole(Point, Point);

    void optimize(custom_node **, polygonHole *);
};

#endif