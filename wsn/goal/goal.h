#pragma once

#include <timer-handler.h>
#include <wsn/versatilerouting_v1/geometry_library/geo_lib.h>
#include <wsn/boundhole/boundhole.h>
#include "goal_packet.h"
#include <time.h>

#define MIN_CAVE_VERTICES 5

using namespace std;

class GoalAgent;

class GoalAgent : public BoundHoleAgent {
private:
    bool already_dump_energy_;
    AgentBroadcastTimer broadcast_timer_;

    std::vector<BoundaryNode> hole;  // hole of this node
    std::vector<std::vector<BoundaryNode>> convex_set_;  // network's convex hull list


    void createHole(Packet *);


    // broadcast phase
    void broadcastHCI();

    void recvHCI(Packet *);


    // routing
    void sendData(Packet *);

    void recvData(Packet *);

    std::vector<BoundaryNode> determineConvexHull(std::vector<BoundaryNode> );

    std::vector<Point> determineCaveContainingNode(Point , std::vector<BoundaryNode> , std::vector<BoundaryNode> );

    Point findNextAnchor(Point, Point);

    void updatePayload(Packet *, std::vector<BoundaryNode> );

    // dump
    void dumpBroadcastRegion();

    void dumpHopCount(hdr_goal_data *);
public:
    GoalAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};