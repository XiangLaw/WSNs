#pragma once

#include "packet.h"
#include "hach.h"

class HACHPacketData : public AppData{
private:
    unsigned char* data_;

public:
    int data_len_;
    int element_size_;

    HACHPacketData();
    HACHPacketData(HACHPacketData &d);	// Copy

    // collect new id
    void add(nsaddr_t id, double x, double y, double x_node, double y_node);

    // get all ids collected
    void dump();

    node get_intersect_data(int index);
    node get_node_data(int index);

    int indexOf(node);
    int indexOf(nsaddr_t id, double x, double y);

    void rmv_data(int index);

    AppData* copy();
    int size() const;
};