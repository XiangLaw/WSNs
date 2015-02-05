#ifndef _ELBAR_H_
#define _ELBAR_H_

#include <vector>
#include "../geomathhelper/geo_math_helper.h"
#include "../gridoffline/gridoffline.h"

enum RoutingMode {
    HOLE_AWARE_MODE,
    GREEDY_MODE,
};

enum Elbar_Region {
    REGION_1, // broadcast boundhole information only
    REGION_2, // broadcast boundhole information
    REGION_3, // greedy only
};

struct angleView {
    struct polygonHole* hole_;
    double angle_;
};

// parallelogram P ABC
struct parallelogram{
    //struct polygonHole* hole;
    struct node p_;
    struct node a_;
    struct node b_;
    struct node c_;
};

class ElbarGridOfflineAgent : public GridOfflineAgent {
private:

    // detect covering parallelogram and view angle
    bool detectParallelogram();
    RoutingMode holeAvoidingProb();
    Elbar_Region regionDetermine(double angle);

    void recvElbar(Packet *p);
    void sendElbar(Packet *p);

    void broadcastHci();   // hole core information broadcast
    void recvHci(Packet *p);        // recv hole core information
    void routing(Packet *p);        // elbar routing algorithm

protected:
    virtual void recvBoundHole(Packet*);

public:
	ElbarGridOfflineAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);

private:
    double alpha_max_;
    double alpha_min_;
    double alpha_;          // alpha angle
    parallelogram* parallelogram_;
    RoutingMode routing_mode_;
    Elbar_Region region_;   // region to a specific hole

public:
    virtual char const * getAgentName();

};

#endif
