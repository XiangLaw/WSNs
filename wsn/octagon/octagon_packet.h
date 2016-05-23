/*
 * octagon_packet.h
 *
 *  Last edited on Nov 22, 2013
 *  by Trong Nguyen
 */

#ifndef OCTAGON_PACKET_H_
#define OCTAGON_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define OCTAGON_DATA_GREEDY  0x01        // packet bring data, forward by greedy
#define OCTAGON_DATA_ROUTING 0x02        // packet bring data, forward by routing table

#define HDR_OCTAGON_DATA(p)    ((struct hdr_octagon_data*)    hdr_octagon::access(p))
#define HDR_OCTAGON_HA(p)    ((struct hdr_octagon_ha*)    hdr_octagon::access(p))
#define HDR_OCTAGON(p)        ((struct hdr_octagon*)        hdr_octagon::access(p))

struct hdr_octagon_data {
    u_int8_t type_;
    nsaddr_t daddr_;
    int vertex_num_;
    Point vertex[8];

    // gpsr header
    u_int8_t gprs_type_;
    Point peri_;
    Point prev_;

    inline int size() {
        return sizeof(nsaddr_t) + sizeof(int) + sizeof(u_int8_t) * 2 + 10 * sizeof(Point);
    }
};

struct hdr_octagon_ha    // hole announcement
{
    int id_;
    Point spos_;
    double pc_;
    double delta_;    // pc_ - p of original hole
    double d_;
    u_int8_t vertex_num_;
    Point vertex[8];

    inline int size() {
        return 9 * sizeof(Point) + sizeof(int) + 3 * sizeof(double) + sizeof(u_int8_t);
    };
};

struct hdr_octagon {
    static int offset_;

    inline static int &offset() { return offset_; }

    inline static struct hdr_octagon *access(const Packet *p) {
        return (struct hdr_octagon *) p->access(offset_);
    }
};

union hdr_all_octagon {
    hdr_octagon heh;
    hdr_octagon_ha hhh;
    hdr_octagon_data hdh;
};

#endif
