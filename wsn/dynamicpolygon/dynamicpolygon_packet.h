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

struct hdr_dynamicpolygon		// boundhole
{
    int n_;
    double g0_, g1_;

	inline int size() { return 2 * sizeof(double) + sizeof(int); }

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_dynamicpolygon* access(const Packet *p)
	{
		return (struct hdr_dynamicpolygon*)p->access(offset_);
	}
};

#endif /* DYNAMICPOLYGON_PACKET_H_ */
