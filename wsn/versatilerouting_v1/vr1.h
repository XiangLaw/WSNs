#pragma once

// VERSION I: broadcast holes' information to all nodes in network

#include <timer-handler.h>
#include <wsn/versatilerouting_v1/geometry_library/geo_lib.h>
#include <wsn/gpsr/gpsr.h>
#include <wsn/common/struct.h>
#include "vr1_packet.h"
#include <time.h>

#define MIN_CAVE_VERTICES 10

using namespace std;



class VR1Agent;

typedef void(VR1Agent::*firefunction)(void);

class VR1Timer : public TimerHandler {
public:
    VR1Timer(VR1Agent *a, firefunction f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);

    VR1Agent *a_;
    firefunction firing_;
};

class VR1Agent : public GPSRAgent {
private:
    friend class BoundHoleHelloTimer;

    VR1Timer findStuck_timer_;
    VR1Timer boundhole_timer_;
    AgentBroadcastTimer broadcast_timer_;

    bool already_dump_energy_;
    stuckangle *stuck_angle_;
    int limit_max_hop_; // limit_boundhole_hop_
    int limit_min_hop_; // min_boundhole_hop_
    int n_;
    int k_n_;
    double theta_n;
    double epsilon_o_;
    double epsilon_i_;
    double network_width_;
    double network_height_;
    corePolygon *core_polygon_set;
    RNG fwdProb_;

    double range_;
    HoleSt hole;  // hole of this node
    std::vector<HoleSt> holes;  // network's holes list

    void startUp();

    void recvVR1(Packet *, Handler *);

    void findStuckAngle();

    void sendBoundHole();

    void recvBoundHole(Packet *);

    void createHole(Packet *);

    node *getNeighborByBoundHole(Point *, Point *);

    // this 2 funcs send and recv hole boundary aproximation packet
    void sendHBA(Packet *);

    void recvHBA(Packet *);


    // broadcast phase
    void broadcastHCI();

    void recvHCI(Packet *);


    // routing
    void sendData(Packet *);

    void recvData(Packet *);



    // outside routing function
    void isNodeStayOnBoundaryOfCorePolygon(Packet *);

    void constructCorePolygonSet(Packet *);

    void addCorePolygonNode(Point, corePolygon *);

    double areaCorePolygon(corePolygon *);

    corePolygon *storeCorePolygons(Packet *);

    std::vector<double> findBasePathPriorityIndex(std::vector<std::vector<BasePathPoint>>);

    std::vector<Point> convexHullOfAllCorePolygons();

    std::vector<Point> scaleOutsidePath(std::vector<BasePathPoint>, double, double, std::vector<CorePolygon> );

    bool isSamePolygon(HoleSt, HoleSt);



    // inside routing function
    std::vector<std::vector<BoundaryNode>> determineConvexHullSet();

    std::vector<Point> determineCaveContainingNode(Point , std::vector<BoundaryNode> , std::vector<BoundaryNode> );

    std::vector<Point> findPath(Point, Point);

    Point findScaleCenter(std::vector<Point>, double, double &);

    int getPathArraySize(Point pth[]);


    // dump
    void dumpBroadcastRegion();

    void dump(Angle, int, int, Line);

    void dumpCorePolygon();

    void dumpNodeInfo();

    void dumpHopCount(hdr_vr1_data *hdr);
public:
    VR1Agent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};