#ifndef VR1_PACKET_H_
#define VR1_PACKET_H_

#include <wsn/geomathhelper/geo_math_helper.h>
#include <packet.h>

#define VR1_BOUNDHOLE    0x01
#define VR1_HBA          0x02
#define VR1_BROADCAST    0x03

#define HDR_VR1_DATA(p)  ((struct hdr_vr1_data*)  hdr_vr1::access(p))
#define HDR_VR1_HA(p)    ((struct hdr_vr1_ha*)    hdr_vr1::access(p))
#define HDR_VR1(p)   ((struct hdr_vr1*)   hdr_vr1::access(p))

struct hdr_vr1_data {
    nsaddr_t source_id_;
    Point dest;
    uint8_t apIndex; // index of anchor point in shortest path from source to destination
    Point path[30];

    uint8_t hopCount;

    inline int size() {
        return 31 * sizeof(Point) + 2 * sizeof(uint8_t) + sizeof(nsaddr_t);
    }
};

struct hdr_vr1_ha {  // hole announcement
    u_int8_t  type_;
    Point prev_;

    int index_; // broadcast use only

    inline int size() {
        return sizeof(u_int8_t) + sizeof(Point) + sizeof(int);
    }
};

struct hdr_vr1 {
    static int offset_;

    inline static int &offset() {
        return offset_;
    }

    inline static struct hdr_vr1 *access(const Packet *p) {
        return (struct hdr_vr1 *) p->access(offset_);
    }
};

union hdr_all_vr1 {
    hdr_vr1 heh;
    hdr_vr1_ha hhh;
    hdr_vr1_data hdh;
};

#endif