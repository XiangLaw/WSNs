/*
 * dynamicpolygon_packet.h
 *
 * Created on: Mar 20, 2014
 * author :    trongnguyen
 */

#ifndef DYNAMICPOLYGON_PACKET_H_
#define DYNAMICPOLYGON_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define HDR_DYNAMICPOLYGON(p) hdr_dynamicpolygon::access(p)

#define hdr_dynamicpolygon_boundhole 	0x00
#define hdr_dynamicpolygon_announcement 0x01

struct hdr_dynamicpolygon		// boundhole
{
	u_int8_t type;	// type of this paket
	Point last_;	// Pre-previews node
	Point prev_;	// Previews node
	Point i_;

	inline int size() { return 3 * sizeof(Point); }

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_dynamicpolygon* access(const Packet *p)
	{
		return (struct hdr_dynamicpolygon*)p->access(offset_);
	}
};

#endif /* DYNAMICPOLYGON_PACKET_H_ */
