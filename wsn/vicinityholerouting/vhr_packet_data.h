#pragma once

#include "packet.h"
#include "vhr.h"

class VHRPacketData : public AppData {
private:
    unsigned char *data_;

public:
    int data_len_;
    int element_size_;

    VHRPacketData();

    VHRPacketData(VHRPacketData &d);    // copy

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