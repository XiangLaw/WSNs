#ifndef _ELBAR_H_
#define _ELBAR_H_

#include <vector>
#include "../geomathhelper/geo_math_helper.h"
#include "../gridoffline/gridoffline.h"


#define HOLE_AWARE_MODE 0x01
#define GREEDY_MODE 0x02
#define GO_TO_DEST_ONLY 0x03

enum Elbar_Region {
    REGION_1 = 1, // broadcast boundhole information only
    REGION_2, // broadcast boundhole information
    REGION_3, // greedy only
};

// parallelogram P ABC
struct parallelogram {
    struct node p_;
    struct node a_;
    struct node b_;
    struct node c_;
};

class ElbarGridOfflineAgent;

class ElbarGridOfflineAgent : public GridOfflineAgent {
private:

    // detect covering parallelogram and view angle
    void detectParallelogram();
    int holeAvoidingProb();
    void regionDetermine(double angle);

    void sendPackageToHop(Packet *p, node *nexthop);
    void configDataPacket(Packet *p);
    void recvData(Packet *p);
    void recvElbar(Packet *p);
    void sendElbar(Packet *p);

    // gpsr
    void sendGPSR(Packet *p);
    node* recvGPSR(Packet *p, Point destination);

    void createGrid(Packet *pPacket);   // re-create grid bound hole
    void broadcastHci();                // hole core information broadcast
    void recvHci(Packet *p);            // recv hole core information
    void routing(Packet *p);            // elbar routing algorithm

    double alpha_max_;
    double alpha_min_;
    double alpha_;          // alpha angle
    parallelogram* parallelogram_;
    int routing_mode_;
    Elbar_Region region_;   // region to a specific hole
                            // convert to struct array for multi hole

    /* math */
    bool isIntersectWithHole(Point *anchor, Point *dest, node* node_list);
    bool isAlphaContainsPoint(Point *x, Point *o, Point *y, Point *d); // check if D is inside xOy
    bool isPointInsidePolygon(Point *d, node* hole);
    /* dump */
    virtual char const *getAgentName();

protected:
    virtual void initTraceFile();

public:
    ElbarGridOfflineAgent();

    int command(int, const char *const *);
    void recv(Packet *, Handler *);

    void dumpAnchorPoint(int id, Point *pPoint);

    void dumpParallelogram();

    void dumpRegion();
};

#endif
