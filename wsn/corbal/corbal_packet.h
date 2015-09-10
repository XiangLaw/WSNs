#ifndef CORBAL_PACKET_H_
#define CORBAL_PACKET_H_

#include "../geomathhelper/geo_math_helper.h"

#define CORBAL_BOUNDHOLE	0x01
#define CORBAL_HBA      	0x02

#define HDR_CORBAL(p) hdr_corbal::access(p)

struct hdr_corbal
{
    u_int8_t type_;
    Point prev_;
    inline int size() { return sizeof(u_int8_t) + sizeof(Point); }

    static int offset_;
    inline static int& offset() { return offset_; }
    inline static struct hdr_corbal* access(const Packet *p)
    {
        return (struct hdr_corbal*)p->access(offset_);
    }
};


#endif