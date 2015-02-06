#ifndef ELBAR_GRID_PACKET_H_
#define ELBAR_GRID_PACKET_H_

#include "packet.h"

#define ELBAR_BROADCAST     0x01
#define ELBAR_DATA          0x02

#define HDR_ELBAR_GRID(p) hdr_elbar_grid::access(p)

struct hdr_elbar_grid {
    Point anchor_point_;
    Point last_;    // Pre-previews node
    Point prev_;    // Previews node
    int forwarding_mode_;
    int type_;

    inline int size() {
        return ( 3 * sizeof(Point) + 2*sizeof(int));
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
