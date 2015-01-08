/*
 * griddynamic_packet.h
 *
 * Created on: Mar 20, 2014
 * author :    trongnguyen
 */

#ifndef GRIDDYNAMIC_PACKET_H_
#define GRIDDYNAMIC_PACKET_H_

#include "packet.h"

// type of packet header
#define GRID_BROADCAST	0x01
#define GRID_ELECTION	0x02
#define GRID_ALARM 		0x03
#define GRID_DATA		0x04

#define HDR_GRIDDYNAMIC(p) hdr_griddynamic::access(p)

struct hdr_griddynamic
{
	u_int8_t type_;				// type of this packet header
	Point p_;					// point of send node | point of candidate | point of send node | point of anchor
	Point d_;
	nsaddr_t id_;				// id of hole | id of candidate | _ | id of destination node
	double value_;				// _ | value of candidate | _ | _

	inline int size()
	{
		switch(type_)
		{
			case GRID_BROADCAST:	return sizeof(u_int8_t) + sizeof(Point) + sizeof(nsaddr_t);
			case GRID_ELECTION:		return sizeof(u_int8_t) + sizeof(Point) + sizeof(nsaddr_t) + sizeof(double);
			case GRID_ALARM:		return sizeof(u_int8_t) + sizeof(Point);
			case GRID_DATA:			return sizeof(u_int8_t) + sizeof(Point) * 2 +  sizeof(nsaddr_t);
		}
		return sizeof(u_int8_t) + sizeof(Point) * 2 +  sizeof(nsaddr_t);
	}

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_griddynamic* access(const Packet *p) {
		return (struct hdr_griddynamic*)p->access(offset_);
	}
};

#endif /* GRIDDYNAMIC_PACKET_H_ */
