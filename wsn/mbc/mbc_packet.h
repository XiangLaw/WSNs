#ifndef MBC_PACKET_H
#define MBC_PACKET_H

#define HDR_MBC(p)	hdr_mbc::access(p)

struct hdr_mbc
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
    inline static struct hdr_mbc* access(const Packet *p)
    {
        return (struct hdr_mbc*) p->access(offset_);
    }
};
#endif