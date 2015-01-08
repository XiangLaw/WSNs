/*
 * grid_packet.h
 *
 * Created on: Mar 20, 2014
 * author :    trongnguyen
 */

#ifndef GRID_PACKET_H_
#define GRID_PACKET_H_

#include "packet.h"

#define HDR_GRID(p) hdr_grid::access(p)

struct hdr_grid
{
	Point last_;	// Pre-previews node
	Point prev_;	// Previews node
	Point i_;

	inline int size() { return 3 * sizeof(Point); }

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct hdr_grid* access(const Packet *p)
	{
		return (struct hdr_grid*)p->access(offset_);
	}
};

#endif /* GRID_PACKET_H_ */
