#ifndef NHR_PACKET_H
#define NHR_PACKET_H

#include <packet.h>

#define HDR_NHR(p) hdr_nhr::access(p)

struct hdr_nhr {
    inline int size()
    {
        return 0;
    }

    static int offset_;
    inline static int& offset()
    {
        return offset_;
    }
    inline static struct hdr_nhr* access(const Packet *p)
    {
        return (struct hdr_nhr*) p->access(offset_);
    }
};

#endif