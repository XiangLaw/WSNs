//
// Created by eleven on 5/8/15.
//

#ifndef NS_GRIDDYNAMIC_PACKET_DATA_H
#define NS_GRIDDYNAMIC_PACKET_DATA_H

#include "packet.h"
#include "wsn/gpsr/gpsr.h"

#define Up 	 	0
#define Left	1
#define Down	2
#define Right	3

class GridDynamicPacketData : public AppData
{
private:
    bool* data_;
    int nx_;
    int ny_;

public:
    GridDynamicPacketData();
    GridDynamicPacketData(GridDynamicPacketData&);

    // collect new id
    void addData(bool**, int, int);
    bool** getData();

    // get all ids collected
    void dump();
    int size() const;

    AppData* copy();
};

#endif //NS_GRIDDYNAMIC_PACKET_DATA_H
