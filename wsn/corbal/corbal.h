#ifndef CORBAL_H_
#define CORBAL_H_

#include <timer-handler.h>
#include <wsn/gpsr/gpsr.h>
#include "../common/struct.h"

class CorbalAgent;

typedef void(CorbalAgent::*firefunction)(void);

class CorbalTimer : public TimerHandler {
public:
    CorbalTimer(CorbalAgent *a, firefunction f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);

    CorbalAgent *a_;
    firefunction firing_;
};

class CorbalAgent : public GPSRAgent {
private:
    friend class BoundHoleHelloTimer;

    CorbalTimer findStuck_timer_;
    CorbalTimer boundhole_timer_;
    AgentBroadcastTimer broadcast_timer_;

    void startUp();

    void findStuckAngle();

    void sendBoundHole();

    void recvBoundHole(Packet *);

    node *getNeighborByBoundHole(Point *, Point *);

    void sendHBA(Packet *);

    void recvHBA(Packet *);

    void contructCorePolygonSet(Packet *);

    void isNodeStayOnBoundaryOfCorePolygon(Packet *);

    void addCorePolygonNode(Point, corePolygon *);

    polygonHole *createPolygonHole(Packet *);

    void broadcastHCI();

    void recvHCI(Packet *);

    void corePolygonSelection(Packet *);

    bool canBroadcast();

    void updatePayload(Packet *); // update payload with new core polygon information

    void sendData(Packet *);

    void recvData(Packet *);

    void calculateScaleFactor(Packet *);

    void findViewLimitVertex(Point *N, corePolygon *, node **, node **);

    double distanceToPolygon(Point *, corePolygon *);

    double euclidLengthOfBRSP(Point *, Point *, corePolygon *);

    void addrouting(Point *p, Point *routingTable, u_int8_t &routingCount);

    void bypassHole(Point *, Point *, corePolygon *, corePolygon *, Point *, u_int8_t &);

    node *recvGPSR(Packet *p, Point destionation);

    double range_;
    int limit_max_hop_; // limit_boundhole_hop_
    int limit_min_hop_; // min_boundhole_hop_
    int n_;
    int k_n_;
    double theta_n;
    double epsilon_;
    double network_width_;
    double network_height_;

    polygonHole *hole_;
    corePolygon *core_polygon_set;
    corePolygon *my_core_polygon;
    stuckangle *stuck_angle_;
    Angle alpha_;
    double l_c_n_;

    double scale_factor_;
    double p_c_;

    void dumpCorePolygon();

    void dump(Angle, int, int, Line);

    void dumpBroadcastRegion();

    void dumpScaleHole(Packet *, corePolygon *);

public:
    CorbalAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};

#endif