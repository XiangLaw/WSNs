#pragma once

#include <wsn/boundhole/boundhole.h>
#include <vector>
#include <algorithm>
#include <map>
#include <set>

using namespace std;

#define MIN_CAVE_VERTEX 10

struct BoundaryNode : Point {
    nsaddr_t id_;
    bool is_convex_hull_boundary_;

    // comparison is done first on y coordinate and then on x coordinate
    bool operator<(BoundaryNode n2) {
        return y_ == n2.y_ ? x_ < n2.x_ : y_ < n2.y_;
    }
};

struct COORDINATE_ORDER {
    bool operator()(const BoundaryNode *a, const BoundaryNode *b) const {
        return a->y_ == b->y_ ? a->x_ < b->x_ : a->y_ < b->y_;
    }
};

// used for sorting points according to polar order w.r.t the pivot
struct POLAR_ORDER {
    POLAR_ORDER(struct BoundaryNode p) { this->pivot = p; }

    bool operator()(const BoundaryNode *a, const BoundaryNode *b) const {
        int order = G::orientation(pivot, *a, *b);
        if (order == 0)
            return G::distance(pivot, *a) < G::distance(pivot, *b);
        return (order == 2);
    }

    struct BoundaryNode pivot;
};

class NHRAgent : public BoundHoleAgent {
private:
    double delta_; // scale factor

    vector<BoundaryNode> hole_;
    vector<BoundaryNode> octagon_hole_;
    AgentBroadcastTimer broadcast_timer_;
    Point endpoint_; // endpoint of the node
    bool isPivot_; // node that can go straight to the gate

    void createHole(Packet *p);

    vector<BoundaryNode> determineConvexHull();

    void approximateHole(vector<BoundaryNode>);

    void broadcastHCI();

    void recvHCI(Packet *);

    bool canBroadcast();

    void calculateScaleFactor(double d);

    double distanceToPolygon(node *polygon);

    void sendData(Packet *p);

    void recvData(Packet *p);

    Point calculateDestEndpoint(Point, int &, int &, int &);

    bool determineOctagonAnchorPoints(Packet *);

    bool sdPolygonIntersect(Packet *);

    void bypassHole(Packet *, Point, Point, vector<BoundaryNode>, Point);

    void findLimitAnchorPoint(Point, vector<BoundaryNode>, Point, int &, int &);

    void findViewLimitVertices(Point, vector<BoundaryNode>, int &, int &);

    bool isPointInsidePolygon(Point, vector<BoundaryNode>);

    void routeToDest(Packet *p);

    void dump();

    void dumpBroadcastRegion();

    void dumpScalePolygon(vector<BoundaryNode>, Point);

public:
    NHRAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};
