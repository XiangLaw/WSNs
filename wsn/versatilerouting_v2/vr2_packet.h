#ifndef VR2_PACKET_H_
#define VR2_PACKET_H_

#include <wsn/geomathhelper/geo_math_helper.h>
#include <packet.h>

#define VR2_BOUNDHOLE    0x01
#define VR2_HBA          0x02
#define VR2_BROADCAST    0x03

#define VR2_CBR_BLIND    0x04
#define VR2_CBR_AWARE    0x05


#define HDR_VR2_DATA(p)  ((struct hdr_vr2_data*)  hdr_vr2::access(p))
#define HDR_VR2_HA(p)    ((struct hdr_vr2_ha*)    hdr_vr2::access(p))
#define HDR_VR2(p)   ((struct hdr_vr2*)   hdr_vr2::access(p))

struct hdr_vr2_data {
    nsaddr_t source_id_;
    Point dest;
    uint8_t apIndex; // index of anchor point in shortest path from source to destination
    Point path[30];

    uint8_t hopCount;
    uint8_t vr2_type_;
    double B_s_t_;

    // gpsr header
    u_int8_t gprs_type_;
    Point peri_;
    Point prev_;

    inline int size() {
        return 33 * sizeof(Point) + 4 * sizeof(uint8_t) + sizeof(double) + sizeof(nsaddr_t);
    }
};

struct hdr_vr2_ha {  // hole announcement
    u_int8_t  type_;
    Point prev_;

    int index_; // broadcast use only

    inline int size() {
        return sizeof(u_int8_t) + sizeof(Point) + sizeof(int);
    }
};

struct hdr_vr2 {
    static int offset_;

    inline static int &offset() {
        return offset_;
    }

    inline static struct hdr_vr2 *access(const Packet *p) {
        return (struct hdr_vr2 *) p->access(offset_);
    }
};

union hdr_all_vr2 {
    hdr_vr2 heh;
    hdr_vr2_ha hhh;
    hdr_vr2_data hdh;
};

#endif