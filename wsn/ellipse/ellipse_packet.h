/*
 * ellipse_packet.h
 *
 * Created on: Jan 6, 2014
 * author :    trongnguyen
 */

#ifndef ELLIPSE_PACKET_H_
#define ELLIPSE_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define HDR_ELLIPSE_BROADCAST(p)((struct hdr_ellipse_broadcast*)hdr_ellipse::access(p))
#define HDR_ELLIPSE_DATA(p) 	((struct hdr_ellipse_data*) 	hdr_ellipse::access(p))
#define HDR_ELLIPSE(p) 			((struct hdr_ellipse*) 			hdr_ellipse::access(p))

struct hdr_ellipse_broadcast
{
	int hole_id_;
	double a_;
	Point F1, F2;		// position of F1 and F2

	inline int size() { return sizeof(int) + sizeof(double) + 2 * sizeof(Point); }
};

struct hdr_ellipse_data {
	int hole_id;		// bypassing hole id
	nsaddr_t daddr_;	// destination address
	Point sou_;			// geometric location of source - only for dump
	Point des_;			// geometric location of destination
	Point sub_;			// location of sub-destination node

	inline int size() { return sizeof(nsaddr_t) + 3 * sizeof(Point); }
};

struct hdr_ellipse {
	u_int8_t type_;

	static int offset_;

	inline static int& offset() { return offset_; }
	inline static struct hdr_ellipse_offset* access(const Packet *p) {
		return (struct hdr_ellipse_offset*)p->access(offset_);
	}
};

union hdr_all_ellipse {
	hdr_ellipse_data eoh;
	hdr_ellipse_broadcast ebh;
	hdr_ellipse eh;
};

#endif
