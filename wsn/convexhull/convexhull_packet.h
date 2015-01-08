/*
 * convexhull_packet.h
 *
 * Created on: Dec 27, 2013
 * author :    trongnguyen
 */

#ifndef CONCVEXHULL_PACKET_H_
#define CONCVEXHULL_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define HDR_CONCVEXHULL(p) hdr_convexhull::access(p)

struct hdr_convexhull
{
	nsaddr_t daddr_;
	Point des;
	Point sub;

	inline int size()
	{
		return sizeof(nsaddr_t) + 2 * sizeof(Point);
	}

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_convexhull* access(const Packet *p) {
		return (struct hdr_convexhull*)p->access(offset_);
	}
};

#endif
