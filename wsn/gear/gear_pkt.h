#ifndef GEAR_PACKET_H_
#define GEAR_PACKET_H_

#include "packet.h"

#define CURRENT Scheduler::instance().clock()
#define INFINITE_DELAY 5000000000000.0

// hello packet header
#define HELLO 	0x01

#define SENDBACK 0x02

// routing packet header
#define GEAR 	0x03

// routing mode
#define PURE 0x01
#define ENERGY 0x02

#define HDR_GEAR(p)   ((struct hdr_gear*)hdr_gear_offset::access(p))
#define HDR_GEAR_HELLO(p) ((struct hdr_gear_hello*)hdr_gear_offset::access(p))
#define HDR_GEAR_SEND_BACK(p) ((struct hdr_gear_send_back*)hdr_gear_offset::access(p))
#define HDR_GEAR_OFFSET(p) ((struct hdr_gear_offset*)hdr_gear_offset::access(p))

struct hdr_gear_hello {
	u_int8_t type_;

	// geo location
	double x_;
	double y_;

	// consumed energy
	double e_;

	inline int size() {
		int sz = sizeof(u_int8_t) + 3 * sizeof(double);
		return sz;
	}
};

struct hdr_gear_send_back {
	u_int8_t type_;

	double h_;

	inline int size() {
		return sizeof(u_int8_t) + sizeof(double);
	}
};

struct hdr_gear {
	u_int8_t type_;

	// geo location of destination node
	double des_x_, des_y_;

	inline int size() { return sizeof(u_int8_t) + 2 * sizeof(double); }
};

struct hdr_gear_offset {
	u_int8_t type_;
	static int offset_;

	inline static int& offset() { return offset_; }
	inline static struct hdr_gear_offset* access(const Packet *p) {
		return (struct hdr_gear_offset*)p->access(offset_);
	}
};

union hdr_all_gear {
	hdr_gear_offset goh;
	hdr_gear_hello hh;
	hdr_gear_send_back sbh;
	hdr_gear gh;
};

#endif /* GEAR_PACKET_H_ */
