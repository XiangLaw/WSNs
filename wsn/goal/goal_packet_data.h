#pragma once

#include <wsn/versatilerouting_v1/geometry_library/geo_lib.h>
#include "packet.h"
#include "wsn/gpsr/gpsr.h"

class GoalPacketData : public AppData {
private:
    unsigned char *data_;

public:
    int data_len_;
    int element_size_;

    GoalPacketData();

    GoalPacketData(GoalPacketData &d);    // Copy

    // collect new id
    void add(nsaddr_t id, double x, double y, bool is_convex_hull_boundary);

    // get all ids collected
    void dump();

    BoundaryNode get_data(int index);

    int indexOf(BoundaryNode);

    int indexOf(nsaddr_t id, double x, double y);

    void rmv_data(int index);

    AppData *copy();

    int size() const;
};
