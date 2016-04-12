/*
 * boundhole_packet.h
 *
 *  Created on: May 26, 2011
 *      Author: leecom
 */

#ifndef BOUNDHOLE_PACKET_H_
#define BOUNDHOLE_PACKET_H_

#include "packet.h"

#define BOUNDHOLE_BOUNDHOLE	0x01
#define BOUNDHOLE_REFRESH	0x02

#define HDR_BOUNDHOLE(p) hdr_boundhole::access(p)

struct hdr_boundhole
{
	u_int8_t type_;
	Point prev_;
	int index_; // for refresh type only
	inline int size() { return sizeof(u_int8_t) + sizeof(Point) + sizeof(index_); }

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_boundhole* access(const Packet *p)
	{
		return (struct hdr_boundhole*)p->access(offset_);
	}
};

#endif /* BOUNDHOLE_PACKET_H_ */
