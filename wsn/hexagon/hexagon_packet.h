/*
 * hexagon_packet.h
 *
 *  Last edited on Nov 22, 2013
 *  by Trong Nguyen
 */

#ifndef HEXAGON_PACKET_H_
#define HEXAGON_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define HEXAGON_DATA_GREEDY  GPSR_GPSR			// packet bring data, forward by greedy
#define HEXAGON_DATA_ROUTING GPSR_PERIME		// packet bring data, forward by routing table

#define HDR_HEXAGON_DATA(p)	((struct hdr_hexagon_data*)	hdr_hexagon::access(p))
#define HDR_HEXAGON_HA(p)	((struct hdr_hexagon_ha*)	hdr_hexagon::access(p))
#define HDR_HEXAGON(p) 		((struct hdr_hexagon*)		hdr_hexagon::access(p))

struct hdr_hexagon_data
{
	u_int8_t type_;
	nsaddr_t daddr_;
	u_int8_t vertex_num_;
	Point vertex[4];

	inline int size()
	{
		return sizeof(nsaddr_t) + 2 * sizeof(u_int8_t) + 4 * sizeof(Point);
	}
};

struct hdr_hexagon_ha
{
	Point spos_;

	int id_;
	Circle circle_;

	inline int size()
	{
		return sizeof(Circle) + sizeof(int) + sizeof(Point);
	};
};

struct hdr_hexagon {
	u_int8_t type_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_hexagon* access(const Packet *p) {
		return (struct hdr_hexagon*)p->access(offset_);
	}
};

union hdr_all_hexagon {
	hdr_hexagon heh;
	hdr_hexagon_ha hhh;
	hdr_hexagon_data hdh;
};

#endif
