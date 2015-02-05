#ifndef ELBAR_GRIDONLINE_PACKET_DATA_H_
#define ELBAR_GRIDONLINE_PACKET_DATA_H_

#include "packet.h"
#include "wsn/gpsr/gpsr.h"

class ElbarGridOfflinePacketData : public AppData
{
private:
    unsigned char* data_;

public:
    int data_len_;
    int element_size_;

    ElbarGridOfflinePacketData();
    ElbarGridOfflinePacketData(ElbarGridOfflinePacketData &d);	// Copy

    // collect new id
    void add_data(double x, double y);

    // get all ids collected
    void dump();

    node get_data(int index);

    int indexOf(node);
    int indexOf(double x, double y);

    void rmv_data(int index);

    AppData* copy();
    int size() const;
};

#endif /* ELBAR_GRIDONLINE_PACKET_DATA_H_ */
