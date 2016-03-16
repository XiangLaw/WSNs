#ifndef NHR_PACKET_DATA_H_
#define NHR_PACKET_DATA_H_

#include "packet.h"
#include "wsn/gpsr/gpsr.h"

class NHRPacketData : public AppData
{
private:
    unsigned char* data_;

public:
    int data_len_;
    int element_size_;

    NHRPacketData();
    NHRPacketData(NHRPacketData &d);	// Copy

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

#endif /* NHR_PACKET_DATA_H_ */
