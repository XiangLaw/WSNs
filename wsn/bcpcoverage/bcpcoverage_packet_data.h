//
// Created by eleven on 9/16/15.
//

#ifndef NS_BCPCOVERAGE_PACKET_DATA_H
#define NS_BCPCOVERAGE_PACKET_DATA_H

#include "packet.h"
#include "bcpcoverage.h"

#define Up 	 	0
#define Left	1
#define Down	2
#define Right	3

class BCPCoveragePacketData : public AppData{
    private:
        unsigned char* data_;

    public:
        int data_len_;
        int element_size_;

    BCPCoveragePacketData();
    BCPCoveragePacketData(BCPCoveragePacketData &d);	// Copy

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
#endif //NS_BCPCOVERAGE_PACKET_DATA_H
