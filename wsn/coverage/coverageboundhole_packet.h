//
// Created by eleven on 9/10/15.
//

#ifndef NS_COVERAGEBOUNDHOLE_PACKET_H
#define NS_COVERAGEBOUNDHOLE_PACKET_H

#define HDR_COVERAGE(p)	hdr_coverage::access(p)

struct hdr_coverage
{
    inline int size()
    {
        return 0;
    }

    static int offset_;
    inline static int& offset()
    {
        return offset_;
    }
    inline static struct hdr_coverage* access(const Packet *p)
    {
        return (struct hdr_coverage*) p->access(offset_);
    }
};

#endif //NS_COVERAGEBOUNDHOLE_PACKET_H
