#ifndef CORBAL_PACKET_H_
#define CORBAL_PACKET_H_

#include "../geomathhelper/geo_math_helper.h"

#define CORBAL_BOUNDHOLE    0x01
#define CORBAL_HBA          0x02
#define CORBAL_BROADCAST    0x03

#define CORBAL_CBR_GREEDY   0x04
#define CORBAL_CBR_ROUTING  0x05

#define HDR_CORBAL(p) hdr_corbal::access(p)

struct hdr_corbal {
    u_int8_t type_;
    Point prev_;
    Point source;
    u_int8_t routing_index;
    Point routing_table[6];

    // gpsr header
    u_int8_t gprs_type_;
    Point peri_;

    int index_; // broadcast use only

    inline int size() {
        return sizeof(u_int8_t) * 3 + 3 * sizeof(Point) + 6 * sizeof(Point) + sizeof(int);
    }

    static int offset_;

    inline static int &offset() { return offset_; }

    inline static struct hdr_corbal *access(const Packet *p) {
        return (struct hdr_corbal *) p->access(offset_);
    }
};


#endif