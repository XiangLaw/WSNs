#ifndef BOUNDHOLE_ROUTING_PACKET_H
#define BOUNDHOLE_ROUTING_PACKET_H

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define BOUNDHOLE_GREEDY	0x01
#define BOUNDHOLE_BOUNDHOLE	0x02

#define HDR_BOUNDHOLEROUTING(p)        ((struct hdr_boundholerouting*)        hdr_boundholerouting::access(p))

struct hdr_boundholerouting {
    u_int8_t type_;
    nsaddr_t daddr_;
    Point dest_;        // destination location
    double distance_;
    inline int size() {
        return sizeof(u_int8_t) + sizeof(nsaddr_t) + sizeof(Point) + sizeof(double);
    }

    static int offset_;

    inline static int &offset() { return offset_; }

    inline static struct hdr_boundholerouting *access(const Packet *p) {
        return (struct hdr_boundholerouting *) p->access(offset_);
    }
};

#endif