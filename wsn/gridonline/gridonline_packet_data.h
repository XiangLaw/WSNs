/**
 * girdonline_packet_data.h
 *
 * Created on: Mar 27, 2014
 * author :    trongnguyen
 */

#ifndef GRIDONLINE_PACKET_DATA_H_
#define GRIDONLINE_PACKET_DATA_H_

#include "packet.h"
#include "wsn/gpsr/gpsr.h"

class GridOnlinePacketData : public AppData
{
	private:
		unsigned char* data_;

	public:
		int data_len_;
		int element_size_;

		GridOnlinePacketData();
		GridOnlinePacketData(GridOnlinePacketData &d);	// Copy

		// add data to tail of list
		void addData(Point p);
		void addData(int index, Point p);

		Point getData(int index);

		void removeData(int index);

		void dump();

		AppData* copy();
		int size() const;
};

#endif /* GRIDONLINE_PACKET_DATA_H_ */
