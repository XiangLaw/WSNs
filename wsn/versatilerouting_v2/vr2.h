#pragma once

// VERSION II: broadcast holes' information to a restrict area in network

#include <timer-handler.h>
#include <wsn/versatilerouting_v1/geometry_library/geo_lib.h>
#include <wsn/gpsr/gpsr.h>
#include <wsn/common/struct.h>
#include "vr2_packet.h"
#include <time.h>

#define MIN_CAVE_VERTICES 10

using namespace std;


class VR2Agent;

typedef void(VR2Agent::*firefunction)(void);

class VR2Timer : public TimerHandler {
public:
    VR2Timer(VR2Agent *a, firefunction f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);

    VR2Agent *a_;
    firefunction firing_;
};

class VR2Agent : public GPSRAgent {
private:
    friend class BoundHoleHelloTimer;

    bool already_dump_energy_;
    VR2Timer findStuck_timer_;
    VR2Timer boundhole_timer_;
    AgentBroadcastTimer broadcast_timer_;

    stuckangle *stuck_angle_;
    int limit_max_hop_; // limit_boundhole_hop_
    int limit_min_hop_; // min_boundhole_hop_
    int n_;
    int k_n_;
    double alpha_;
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

    void recvVR2(Packet *);

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

    bool canBroadcast(double, corePolygon *);

    HoleSt storeCorePolygons(Packet *, corePolygon *, bool &);

    std::vector<BoundaryNode> determineConvexHull(HoleSt);

    double distanceToPolygon(node *polygon);



    // routing
    void sendData(Packet *);

    void recvData(Packet *);


    // outside routing function
    void isNodeStayOnBoundaryOfCorePolygon(Packet *);

    void constructCorePolygonSet(Packet *);

    void addCorePolygonNode(Point, corePolygon *);

    double areaCorePolygon(corePolygon *);

    std::vector<double> findBasePathPriorityIndex(std::vector<std::vector<BasePathPoint>>);

    std::vector<Point> convexHullOfAllCorePolygons();

    std::vector<Point> scalePath(std::vector<std::vector<Point>>, double, Point, Point, std::vector<HoleSt> hole_list_,
                                 std::vector<CorePolygon> obs);

    void devideOutsidePath(std::vector<BasePathPoint> , Point, Point, std::vector<std::vector<Point>> &);

    void devideInsidePath(std::vector<InsidePoint>, Point, Point, std::vector<std::vector<Point>> & );

    bool segmentCutHoleOrCore(Point, Point, std::vector<HoleSt> hole_list_);

    bool isSamePolygon(HoleSt, HoleSt);

    void findPath(Packet *);
    void findPath2(Packet *p);

    bool hasDestHoleInfo(Point);

    node *recvGPSR(Packet *p, Point destionation);


    // inside routing function
    void determineConvexHullSet(std::vector<std::vector<BoundaryNode>> &);

    std::vector<InsidePoint> determineCaveContainingNode(Point, std::vector<BoundaryNode>, std::vector<BoundaryNode>);

    // utility
    double calculateM(CorePolygon);         // thong so quyet dinh vung broadcast - xem trong bai bao


    // dump
    void dumpBroadcastRegion();

    void dump(Angle, int, int, Line);

    void dumpCorePolygon();

    void dumpNodeInfo();

    void dumpHopCount(hdr_vr2_data *hdr);

    void dumpDrop(int saddr_, int type_);

    int positionSegHole(Point p1, Point p2, HoleSt);

public:
    VR2Agent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};