#ifndef TAAGENT_PACKET_H_
#define TAAGENT_PACKET_H_

#define HDR_TAAGENT(p)	hdr_taagent::access(p)

struct hdr_taagent
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
    inline static struct hdr_taagent* access(const Packet *p)
    {
        return (struct hdr_taagent*) p->access(offset_);
    }
};

#endif