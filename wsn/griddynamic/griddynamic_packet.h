#ifndef GRIDDYNAMIC_PACKET_H_
#define GRIDDYNAMIC_PACKET_H_

#include "packet.h"

// type of packet header
#define GRID_PIVOT		0x01    // packet send to pivot to notify about pivot info
#define GRID_NOTIFY		0x02    // packet send to all node in cell to notify about pivot
#define GRID_UPDATE		0x03    // packet from node in cell send to pivot to update status
#define GRID_COLLECT	0x04    // packet send to sink/master node

#define HDR_GRIDDYNAMIC(p) hdr_griddynamic::access(p)

struct hdr_griddynamic
{
	u_int8_t type_;				// type of this packet header
	nsaddr_t id_;				// id of hole | id of candidate | _ | id of destination node
	Point p_;					// point of send node | point of candidate | point of send node | point of anchor

	inline int size()
	{
//		switch(type_)
//		{
//			case GRID_BROADCAST:	return sizeof(u_int8_t) + sizeof(Point) + sizeof(nsaddr_t);
//			case GRID_ELECTION:		return sizeof(u_int8_t) + sizeof(Point) + sizeof(nsaddr_t) + sizeof(double);
//			case GRID_ALARM:		return sizeof(u_int8_t) + sizeof(Point);
//			case GRID_DATA:			return sizeof(u_int8_t) + sizeof(Point) * 2 +  sizeof(nsaddr_t);
//		}
		return sizeof(u_int8_t) + sizeof(Point) +  sizeof(nsaddr_t);
	}

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_griddynamic* access(const Packet *p) {
		return (struct hdr_griddynamic*)p->access(offset_);
	}
};

#endif /* GRIDDYNAMIC_PACKET_H_ */
