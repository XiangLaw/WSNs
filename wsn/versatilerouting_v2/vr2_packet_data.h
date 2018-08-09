#pragma once

#include <ns-process.h>
#include <wsn/geomathhelper/geo_math_helper.h>

class VR2PacketData: public AppData {
private:
    unsigned char *data_;

public:
    VR2PacketData();
    VR2PacketData(VR2PacketData &d);  // copy

    int data_len_;
    int element_size_;

    // collect new id
    void add(nsaddr_t id, double x, double y);

    // collect hba information
    void addHBA(int n, int kn);

    // add B(i) node information to HBA
    void addBiNode(int offset, nsaddr_t id, double x, double y);

    // get all ids collected
    void dump();

    node get_data(int index);
    node get_Bi_data(int);
    int get_next_index_of_Bi(int);
    void update_next_index_of_Bi(int, int);

    int indexOf(node);
    int indexOf(nsaddr_t id, double x, double y);

    void rmv_data(int index);

    AppData* copy();
    int size() const;
};