/**
*
*/

#ifndef ELBAR_GRIDONLINE_PACKET_DATA_H_
#define ELBAR_GRIDONLINE_PACKET_DATA_H_

#include "packet.h"
#include "wsn/gpsr/gpsr.h"

class ElbarGridOnlinePacketData : public AppData
{
private:
    unsigned char* data_;

public:
    int data_len_;
    int element_size_;

    ElbarGridOnlinePacketData();
    ElbarGridOnlinePacketData(ElbarGridOnlinePacketData &d);	// Copy

    // add data to tail of list
    void addData(Point p);
    void addData(int index, Point p);

    Point getData(int index);

    void removeData(int index);

    void dump();

    AppData* copy();
    int size() const;
};

#endif /* ELBAR_GRIDONLINE_PACKET_DATA_H_ */
