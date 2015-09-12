#ifndef CORBAL_H_
#define CORBAL_H_

#include <timer-handler.h>
#include <wsn/gpsr/gpsr.h>
#include "../common/struct.h"

class CorbalAgent;
typedef void(CorbalAgent::*firefunction)(void);

class CorbalTimer : public TimerHandler
{
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

    void startUp();

    void findStuckAngle();

    void sendBoundHole();
    void recvBoundHole(Packet*);

    void sendHBA(Packet*);
    void recvHBA(Packet*);
    void contructCorePolygonSet(Packet *);
    void isNodeStayOnBoundaryOfCorePolygon(Packet*);
    void addCorePolygonNode(Point, corePolygon*);
    polygonHole* createPolygonHole(Packet*);

    void broadcastHCI(Packet *);
    void recvHCI(Packet *);

    node* getNeighborByBoundHole(Point*, Point*);


    void sendData(Packet*);
    void recvData(Packet*);

    double range_;
    int limit_max_hop_; // limit_boundhole_hop_
    int limit_min_hop_; // min_boundhole_hop_
    int n_;
    int kn;
    double theta_n;
    polygonHole *hole_;
    corePolygon *core_polygon_set;

    stuckangle* stuck_angle_;

    void dumpCorePolygon();
    void dump(Angle, int, int, Line);
public:
    CorbalAgent();
    int 	command(int, const char*const*);
    void 	recv(Packet*, Handler*);

};

#endif