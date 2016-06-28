#ifndef GPSR_PACKET_H_
#define GPSR_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define GPSR_GPSR	0x01
#define GPSR_PERIME	0x02

#define HDR_HELLO(p)	hdr_hello::access(p)
#define HDR_GPSR(p) 	hdr_gpsr::access(p)

struct hdr_hello
{
	Point location_;

	inline int size()
	{
		return sizeof(Point);
	}

	static int offset_;
	inline static int& offset()
	{
		return offset_;
	}
	inline static struct hdr_hello* access(const Packet *p)
	{
		return (struct hdr_hello*) p->access(offset_);
	}
};

struct hdr_gpsr
{
	u_int8_t type_;
	nsaddr_t daddr_;

	Point peri_;		// location enter perimeter node
	Point dest_;		// destination location
	Point prev_;		// position of previous node
	Point routing_table_[3];

	inline int size()
	{
		return sizeof(u_int8_t) + sizeof(nsaddr_t) + 6 * sizeof(Point);
	}

	static int offset_;
	inline static int& offset()
	{
		return offset_;
	}
	inline static struct hdr_gpsr* access(const Packet *p)
	{
		return (struct hdr_gpsr*) p->access(offset_);
	}
};

#endif
