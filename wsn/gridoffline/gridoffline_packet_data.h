/*
 * gridoffline_packet_data.h
 *
 * Created on: Mar 29, 2014
 * author :    trongnguyen
 */

#ifndef GRIDOFFLINE_PACKET_DATA_H_
#define GRIDOFFLINE_PACKET_DATA_H_

#include "packet.h"
#include "wsn/gpsr/gpsr.h"

#define Up 	 	0
#define Left	1
#define Down	2
#define Right	3

class GridOfflinePacketData : public AppData
{
	private:
		char* data_;
		int data_len_;

	public:
		GridOfflinePacketData();
		GridOfflinePacketData(GridOfflinePacketData&);

		// collect new id
		void addData(char e);
		char getData(int index);

		// get all ids collected
		void dump();
		int size() const;

		AppData* copy();
};

#endif /* GRIDOFFLINE_PACKET_DATA_H_ */
