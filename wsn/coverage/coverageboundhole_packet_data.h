//
// Created by eleven on 9/16/15.
//

#ifndef NS_COVERAGEBOUNDHOLE_PACKET_DATA_H
#define NS_COVERAGEBOUNDHOLE_PACKET_DATA_H

#include "packet.h"
#include "coverageboundhole.h"

class CoverageBoundHolePacketData : public AppData{
    private:
        unsigned char* data_;

    public:
        int data_len_;
        int element_size_;

    CoverageBoundHolePacketData();
    CoverageBoundHolePacketData(CoverageBoundHolePacketData &d);	// Copy

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
#endif //NS_COVERAGEBOUNDHOLE_PACKET_DATA_H
