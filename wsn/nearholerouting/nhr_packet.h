#pragma once

#include <packet.h>
#include <wsn/geomathhelper/geo_math_helper.h>

#define HDR_NHR(p) hdr_nhr::access(p)

#define NHR_CBR_GPSR 1
#define NHR_CBR_AWARE_SOURCE_ESCAPE 2
#define NHR_CBR_AWARE_SOURCE_PIVOT 3
#define NHR_CBR_AWARE_OCTAGON 4
#define NHR_CBR_AWARE_DESTINATION 5

struct hdr_nhr {
    nsaddr_t daddr_;
    Point dest_;
    Point anchor_points[10];
    // 0. dest's endpoint
    // 1. source's endpoint
    // 2 -> 7. octagon vertices
    uint8_t ap_index;
    uint8_t type;
    int dest_level;

    inline int size() {
        return 11 * sizeof(Point) + 2 * sizeof(uint8_t) + sizeof(nsaddr_t) + sizeof(int);
    }

    static int offset_;

    inline static int &offset() {
        return offset_;
    }

    inline static struct hdr_nhr *access(const Packet *p) {
        return (struct hdr_nhr *) p->access(offset_);
    }
};
