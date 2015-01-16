/* Greedy routing forwarding sample 
*	@author pvhau
*/

#ifndef __greedy_pkt_h__
#define __greedy_pkt_h__

#include "packet.h"

#define HDR_GREEDY(p)   ((struct hdr_greedy*)hdr_greedy::access(p))
#define HDR_GREEDY_DATA(p) ((struct hdr_greedy_data*)hdr_greedy::access(p))

enum GreedyPktType {
	GREEDY_PKT_DATA		// data
};

struct hdr_greedy {
	GreedyPktType type_;
	
	static int offset_;
	inline static int& offset() {return offset_;}
	inline static hdr_greedy* access(const Packet* p) {
		return (hdr_greedy*) p->access(offset_);
	}
};

struct hdr_greedy_data {
	GreedyPktType type_;				// the type of pkt
	nsaddr_t src_;						// the src addr
	nsaddr_t dest_;						// the dest addr
	float destX_;						// the location X of the dest
	float destY_;						// the location Y of the dest

	inline int size() {
		unsigned int s = 2 * sizeof(int) + 1 + 2 * sizeof(float);
		return s;
	}
};

union hdr_all_greedy {
	hdr_greedy greedyHdr;
	hdr_greedy_data greedyHdrData;
};

#endif
