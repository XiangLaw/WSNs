//
// Created by Vu Quoc Huy  on 8/27/15.
//

#ifndef BEHDS_PACKET_H
#define BEHDS_PACKET_H

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define BEHDS_DATA_GREEDY  0x01			// packet bring data, forward by greedy
#define BEHDS_DATA_ROUTING 0x02		// packet bring data, forward by routing table
#define BEHDS_DATA_HA      0x03         // packet bring data, forward back to source with hole info

#define HDR_BEHDS_DATA(p) 	((struct hdr_behds_data*)hdr_behds::access(p))
#define HDR_BEHDS_HA(p)		((struct hdr_behds_ha*)	hdr_behds::access(p))
#define HDR_BEHDS(p) 		((struct hdr_behds*)		hdr_behds::access(p))

struct hdr_behds_data
{
    u_int8_t type_;
    nsaddr_t daddr_;
    u_int8_t vertex_num_;
    Point vertex[3];
    nsaddr_t saddr;
    nsaddr_t daddr;
    Point routing_table_[3];

    inline int size()
    {
        return 3 * sizeof(nsaddr_t) + 2 * sizeof(u_int8_t) + 6 * sizeof(Point);
    }
};

struct hdr_behds_ha
{
    u_int8_t type_;

    Point spos_;

    int id_;
    Circle circle_;
    nsaddr_t saddr;
    nsaddr_t daddr;

    inline int size()
    {
        return sizeof(Circle) + sizeof(int) + sizeof(Point) + sizeof(u_int8_t) + 2 * sizeof(nsaddr_t);
    };
};

struct hdr_behds {
    u_int8_t type_;

    static int offset_;
    inline static int& offset() { return offset_; }
    inline static struct hdr_behds* access(const Packet *p) {
        return (struct hdr_behds*)p->access(offset_);
    }
};

union hdr_all_behds {
    hdr_behds heh;
    hdr_behds_ha hhh;
    hdr_behds_data hdh;
};

#endif //BEHDS_PACKET_H
