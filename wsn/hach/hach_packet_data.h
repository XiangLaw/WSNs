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
    void add(nsaddr_t id, double x, double y);

    // get all ids collected
    void dump();

    node get_data(int index);

    int indexOf(node);
    int indexOf(nsaddr_t id, double x, double y);

    void rmv_data(int index);

    AppData* copy();
    int size() const;
};