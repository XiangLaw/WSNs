#ifndef _ELBAR_PACKET_H_
#define _ELBAR_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define HDR_HELLO(p)	hdr_hello::access(p)
#define HDR_GPSR(p) 	hdr_elbar::access(p)

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

struct hdr_elbar
{
	u_int8_t type_;
	nsaddr_t daddr_;

	Point peri_;		// location enter perimeter node
	Point dest_;		// destination location
	Point prev_;		// position of previous node

	inline int size()
	{
		return sizeof(u_int8_t) + sizeof(nsaddr_t) + 3 * sizeof(Point);
	}

	static int offset_;
	inline static int& offset()
	{
		return offset_;
	}
	inline static struct hdr_elbar* access(const Packet *p)
	{
		return (struct hdr_elbar*) p->access(offset_);
	}
};

#endif