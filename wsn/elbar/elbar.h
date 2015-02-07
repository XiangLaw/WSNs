#ifndef _ELBAR_H_
#define _ELBAR_H_

#include <vector>
#include "../geomathhelper/geo_math_helper.h"
#include "../gridoffline/gridoffline.h"


#define HOLE_AWARE_MODE 0x01
#define GREEDY_MODE 0x02

enum Elbar_Region {
    REGION_1, // broadcast boundhole information only
    REGION_2, // broadcast boundhole information
    REGION_3, // greedy only
};

struct angleView {
    int hole_id_;
    double angle_;
    struct angleView *next_;

    ~angleView() {
        angle_ = 0;
    }
};

// parallelogram P ABC
struct parallelogram {
    struct node p_;
    struct node a_;
    struct node b_;
    struct node c_;
    struct parallelogram *next_;
};

class ElbarGridOfflineAgent;

typedef void(ElbarGridOfflineAgent::*firefunc)(void);

class ElbarGridOfflineTimer : public TimerHandler {
public:
    ElbarGridOfflineTimer(ElbarGridOfflineAgent *a, firefunc f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);

    ElbarGridOfflineAgent *a_;
    firefunc firing_;
};

class ElbarGridOfflineAgent : public GridOfflineAgent {
private:
    friend class ElbarGridOfflineTimer;

    // detect covering parallelogram and view angle
    void detectParallelogram();
    int holeAvoidingProb();
    Elbar_Region regionDetermine(double angle);

    void configDataPacket(Packet *p);
    void recvData(Packet *p);
    void recvElbar(Packet *p);
    void sendElbar(Packet *p);

    void broadcastHci();            // hole core information broadcast
    void recvHci(Packet *p);        // recv hole core information
    void routing(Packet *p);        // elbar routing algorithm

    virtual char const *getAgentName();
    void dumpAngle();
    void dumpNodeInfoX();
    void dumpNodeInfoY();
    void createGrid(Packet *pPacket); // re-create grid bound hole

protected:
    virtual void initTraceFile();

public:
    ElbarGridOfflineAgent();

    int command(int, const char *const *);
    void recv(Packet *, Handler *);

private:
    double alpha_max_;
    double alpha_min_;
    angleView *alpha_;          // alpha angle
    parallelogram *parallelogram_;
    int routing_mode_;
    Elbar_Region region_;   // region to a specific hole
                            // convert to struct array for multi hole

    ElbarGridOfflineTimer broadcast_timer_;
};

#endif
