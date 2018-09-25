#ifndef EDGR_PACKET_H_
#define EDGR_PACKET_H_

#include <wsn/geomathhelper/geo_math_helper.h>
#include <packet.h>

#define EDGR_BURST_GREEDY   0x01
#define EDGR_BURST_BYPASS   0x02
#define EDGR_BURST_FLAG_LEFT    0x03
#define EDGR_BURST_FLAG_RIGHT   0x04
#define EDGR_BURST_FORWARD   0x05
#define EDGR_BURST_FEED_BACK    0x06


#define HDR_BEACON(p)   hdr_beacon::access(p)
#define HDR_BURST(p)   hdr_burst::access(p)
#define HDR_EDGR(p)     hdr_edgr::access(p)

struct hdr_beacon
{
    Point location_;
    float_t residual_energy_;

    inline int size()
    {
        return sizeof(Point) + sizeof(float_t);
    }

    static int offset_;
    inline static int& offset()
    {
        return offset_;
    }
    inline static struct hdr_beacon* access(const Packet *p)
    {
        return (struct hdr_beacon*) p->access(offset_);
    }
};


struct hdr_burst
{
    u_int8_t mode_;     // greedy mode or bypass mode
    u_int8_t flag_;      // left or right path (bases on left/right hand rule)
    u_int8_t direction_;

    Point void_;        // location enter perimeter mode
    Point dest_;        // destination location
    Point prev_;        // position of previous node
    Point source_;

    Point anchor_list_[20];
    u_int8_t anchor_index_;

    inline int size()
    {
        return 3 * sizeof(u_int8_t) + 24 * sizeof(Point) + sizeof(nsaddr_t);
    }

    static int offset_;
    inline static int& offset()
    {
        return offset_;
    }
    inline static struct hdr_burst* access(const Packet *p)
    {
        return (struct hdr_burst*) p->access(offset_);
    }
};


struct hdr_edgr
{
    Point anchor_list_[20];
    nsaddr_t dest_addr_;

    inline int size()
    {
        return 20 * sizeof(Point) + sizeof(nsaddr_t);
    }

    static int offset_;
    inline static int& offset()
    {
        return offset_;
    }
    inline static struct hdr_edgr* access(const Packet *p)
    {
        return (struct hdr_edgr*) p->access(offset_);
    }
};

#endif

