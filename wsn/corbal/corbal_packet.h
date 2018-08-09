#ifndef CORBAL_PACKET_H_
#define CORBAL_PACKET_H_

#include "../geomathhelper/geo_math_helper.h"

#define CORBAL_BOUNDHOLE    0x01
#define CORBAL_HBA          0x02
#define CORBAL_BROADCAST    0x03

#define CORBAL_CBR_GREEDY   0x04
#define CORBAL_CBR_ROUTING  0x05

#define HDR_CORBAL_DATA(p)    ((struct hdr_corbal_data*)    hdr_corbal::access(p))
#define HDR_CORBAL_HA(p)    ((struct hdr_corbal_ha*)    hdr_corbal::access(p))
#define HDR_CORBAL(p)        ((struct hdr_corbal*)        hdr_corbal::access(p))

struct hdr_corbal_data {
    u_int8_t type_;
    Point prev_;
    Point source;
    nsaddr_t routing_index;
    Point routing_table[20];
    Point sub;
    nsaddr_t sub_count;
    nsaddr_t hopcount_;

    // gpsr header
    u_int8_t gprs_type_;
    Point peri_;

    inline int size() {
        return sizeof(u_int8_t) * 3 + 3 * sizeof(Point) + 21 * sizeof(Point);
    }
};

struct hdr_corbal_ha    // hole announcement
{
    u_int8_t type_;
    Point prev_;

    int index_; // broadcast use only

    inline int size() {
        return sizeof(u_int8_t) + sizeof(Point) + sizeof(int);
    }
};

struct hdr_corbal {
    static int offset_;

    inline static int &offset() { return offset_; }

    inline static struct hdr_corbal *access(const Packet *p) {
        return (struct hdr_corbal *) p->access(offset_);
    }
};

union hdr_all_corbal {
    hdr_corbal heh;
    hdr_corbal_ha hhh;
    hdr_corbal_data hdh;
};

#endif
