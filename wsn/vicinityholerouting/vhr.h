#pragma once

#include <wsn/boundhole/boundhole.h>

#define MIN_CAVE_VERTICES 10

using namespace std;

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

class VHRAgent: public BoundHoleAgent {
private:
    double broadcastAngle;
    double scaleFactor;
    std::vector<BoundaryNode> hole;
    AgentBroadcastTimer broadcast_timer_;

    // broadcast phase - disseminate hole's information
    void broadcastHCI();
    void recvHCI(Packet *);
    bool canBroadcast();
    void createHole(Packet *p);


    // routing - handling data packet
    void sendData(Packet *);
    void recvData(Packet *);


    // geometric function - to find shortest path between source - destination
    int getVertexIndexInPolygon(Point, std::vector<Point>);
    double calculatePathLength(std::vector<Point>);
    std::vector<BoundaryNode> determineConvexHull();
    double findViewAngle(Point, std::vector<BoundaryNode>);
    void findViewLimitVertices(Point, std::vector<BoundaryNode>, int &i1, int &i2);
    std::vector<Point> determineCaveContainingNode(Point s);
    std::vector<Point> bypassHole(double, double, double, double, int, int, int, int, std::vector<Point>);
    std::vector<Point> findPath(Point, Point);
    Point findHomotheticCenter(std::vector<Point>, double, double &);
    int getPathArraySize(Point pth[]);


    // dump
    void dumpBroadcastRegion();

public:
    VHRAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};