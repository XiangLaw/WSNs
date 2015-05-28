#ifndef ELBAR_GRID_PACKET_H_
#define ELBAR_GRID_PACKET_H_

#include "packet.h"

#define ELBAR_BROADCAST     0x01
#define ELBAR_DATA          0x02

#define HDR_ELBAR_GRID(p) hdr_elbar_grid::access(p)

struct hdr_elbar_grid {
    int forwarding_mode_;
    int type_;
    nsaddr_t daddr; // destination address
    Point anchor_point_;
    Point destination_;    // destionantion node of simulation

    // gpsr header
    u_int8_t gprs_type_;
    Point peri_;
    Point prev_;

    inline int size() {
        return ( 4 * sizeof(Point) + 2*sizeof(int) + sizeof(nsaddr_t) + sizeof(u_int8_t));
    }

    static int offset_;

    inline static int &offset() {
        return offset_;
    }

    inline static struct hdr_elbar_grid *access(const Packet *p) {
        return (struct hdr_elbar_grid *) p->access(offset_);
    }
};

#endif /* ELBAR_GRID_PACKET_H_ */
