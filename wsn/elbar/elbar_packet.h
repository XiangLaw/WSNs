#ifndef ELBAR_GRIDONLINE_H_
#define ELBAR_GRIDONLINE_H_

#include "packet.h"
#include "elbar.h"

#define ELBAR_BOUNDHOLE     0x01

#define HDR_ELBAR_GRID(p) hdr_elbar_gridonline::access(p)

struct hdr_elbar_gridonline
{
    RoutingMode forwarding_mode_;
    Point anchor_point_;

    uint8_t type_;

    inline int size() { return sizeof(RoutingMode) + sizeof(Point) + sizeof(uint8_t); }

    static int offset_;
    inline static int& offset() { return offset_; }
    inline static struct hdr_elbar_gridonline* access(const Packet *p)
    {
        return (struct hdr_elbar_gridonline*)p->access(offset_);
    }
};

#endif /* ELBAR_GRIDONLINE_H_ */
