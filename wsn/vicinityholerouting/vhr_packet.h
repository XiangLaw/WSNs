#pragma once

#include <packet.h>
#include <wsn/geomathhelper/geo_math_helper.h>

#define HDR_VHR(p) hdr_vhr::access(p)

struct hdr_vhr {
    Point dest;
    uint8_t apIndex; // index of anchor point in shortest path from source to destination
    Point path[20];

    inline int size() {
        return 21 * sizeof(Point) + sizeof(uint8_t);
    }

    static int offset_;

    inline static int &offset() {
        return offset_;
    }

    inline static struct hdr_vhr *access(const Packet *p) {
        return (struct hdr_vhr *) p->access(offset_);
    }
};