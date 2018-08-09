#ifndef GOAL_PACKET_H_
#define GOAL_PACKET_H_

#include <wsn/geomathhelper/geo_math_helper.h>
#include <packet.h>

#define GOAL_GLOBAL 0x01
#define GOAL_LOCAL  0x02

#define HDR_GOAL_DATA(p)    ((struct hdr_goal_data*)    hdr_goal::access(p))
#define HDR_GOAL_HA(p)  ((struct hdr_goal_ha*)  hdr_goal::access(p))
#define HDR_GOAL(p) ((struct hdr_goal*) hdr_goal::access(p))

struct hdr_goal_data {
    nsaddr_t source_id_;
    Point dest;
    Point anchor;

    uint8_t hopCount;

    inline int size() {
        return 2 * sizeof(Point) + sizeof(uint8_t) + sizeof(nsaddr_t );
    }
};

struct hdr_goal_ha {
    u_int8_t type_;

    inline int size() {
        return sizeof(u_int8_t);
    }
};

struct hdr_goal {
    static int offset_;

    inline static int &offset() {
        return offset_;
    }

    inline static struct hdr_goal *access(const Packet *p) {
        return (struct hdr_goal *) p->access(offset_);
    }
};

union hdr_all_goal {
    hdr_goal heh;
    hdr_goal_ha hhh;
    hdr_goal_data hdh;
};
#endif