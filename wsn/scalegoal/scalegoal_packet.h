/*
 * scalegoal_packet.h
 *
 * Created on: Dec 27, 2013
 * author :    trongnguyen
 */

#ifndef SCALEGOAL_PACKET_H_
#define SCALEGOAL_PACKET_H_

#include "packet.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#define HDR_SCALEGOAL(p) hdr_scalegoal::access(p)

struct hdr_scalegoal
{
	int hole_id_;
	int level_;
	double g;

	inline int size()
	{
		return 2 * sizeof(int) + sizeof(double);
	}

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_scalegoal* access(const Packet *p) {
		return (struct hdr_scalegoal*)p->access(offset_);
	}
};

#endif
