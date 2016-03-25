#ifndef MBC_PACKET_DATA_H
#define MBC_PACKET_DATA_H

#include "packet.h"

class MbcPacketData : public AppData {
private:
    unsigned char *data_;

public:
    int data_len_;
    int element_size_;

    MbcPacketData();

    MbcPacketData(MbcPacketData &d);    // Copy

    // collect new id
    void add(nsaddr_t id, double x, double y, double x_node, double y_node);

    // get all ids collected
    void dump();

    node get_intersect_data(int index);

    node get_node_data(int index);

    int indexOf(node);

    int indexOf(nsaddr_t id, double x, double y);

    void rmv_data(int index);

    AppData *copy();

    int size() const;
};

#endif