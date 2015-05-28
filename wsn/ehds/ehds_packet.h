/*
 * ehds_packet.h
 *
 *  Last edited on Nov 22, 2013
 *  by Trong Nguyen
 */

#ifndef EHDS_PACKET_H_
#define EHDS_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define EHDS_DATA_GREEDY  GPSR_GPSR			// packet bring data, forward by greedy
#define EHDS_DATA_ROUTING GPSR_PERIME		// packet bring data, forward by routing table

#define EHDS_HA 0x01						// packet bring hole information to special node
#define EHDS_BC 0x01						// packet broadcast hole information

#define HDR_EHDS_DATA(p) 	((struct hdr_ehds_data*)hdr_ehds::access(p))
#define HDR_EHDS_HA(p)		((struct hdr_ehds_ha*)	hdr_ehds::access(p))
#define HDR_EHDS(p) 		((struct hdr_ehds*)		hdr_ehds::access(p))

struct hdr_ehds_data
{
	u_int8_t type_;
	nsaddr_t daddr_;
	u_int8_t vertex_num_;
	Point vertex[3];

	inline int size()
	{
		return sizeof(nsaddr_t) + 2 * sizeof(u_int8_t) + 3 * sizeof(Point);
	}
};

struct hdr_ehds_ha
{
	u_int8_t type_;
	Point spos_;

	int id_;
	Circle circle_;

	inline int size()
	{
		return sizeof(Circle) + sizeof(int) + sizeof(Point) + sizeof(u_int8_t);
	};
};

struct hdr_ehds {
	u_int8_t type_;

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_ehds* access(const Packet *p) {
		return (struct hdr_ehds*)p->access(offset_);
	}
};

union hdr_all_ehds {
	hdr_ehds heh;
	hdr_ehds_ha hhh;
	hdr_ehds_data hdh;
};

#endif
