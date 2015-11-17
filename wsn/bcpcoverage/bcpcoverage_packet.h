//
// Created by eleven on 10/7/15.
//

#ifndef NS_BCPCOVERAGE_PACKET_H
#define NS_BCPCOVERAGE_PACKET_H

#define HDR_BCPCOVERAGE(p)	hdr_bcpcoverage::access(p)

struct hdr_bcpcoverage
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
    inline static struct hdr_bcpcoverage* access(const Packet *p)
    {
        return (struct hdr_bcpcoverage*) p->access(offset_);
    }
};

#endif //NS_BCPCOVERAGE_PACKET_H
