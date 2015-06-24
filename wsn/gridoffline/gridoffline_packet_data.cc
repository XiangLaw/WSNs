/*
 * grid_packet_data.cc
 *
 * Created on: Mar 20, 2014
 * author :    trongnguyen
 */

#include "common/ns-process.h"
#include "gridoffline_packet_data.h"
#include "cstring"

GridOfflinePacketData::GridOfflinePacketData() : AppData(GRIDOFFLINE_DATA)
{
	data_ = NULL;
	data_len_ = 0;
}

GridOfflinePacketData::GridOfflinePacketData(GridOfflinePacketData &d) : AppData(d)
{
	data_len_ = d.data_len_;

	if (data_len_ > 0)
	{
		data_ = new char[data_len_];
		memcpy(data_, d.data_, data_len_);
	}
	else
	{
		data_ = NULL;
	}
}

void GridOfflinePacketData::addData(char e)
{
	char* temp = data_;
	data_ = new char[data_len_ + sizeof(char)];

	memcpy(data_, temp, data_len_);
	memcpy(data_ + data_len_, &e, sizeof(char));

	data_len_ += sizeof(char);
}

char GridOfflinePacketData::getData(int index)
{
	char re;
	int offset = (index - 1) * (sizeof(char));
	
	memcpy(&re, data_ + offset,  sizeof(char));

	return re;
}

int GridOfflinePacketData::size() const {
	return data_len_ / sizeof(char);
}

void GridOfflinePacketData::dump()
{
	FILE *fp = fopen("DataDump.tr", "w");
	for (int i = 1; i <= data_len_ / sizeof(char); i++)
	{
		char n = getData(i);
		fprintf(fp, "%d\n", n);
	}
	fprintf(fp, "\n");
	fclose(fp);
}

AppData* GridOfflinePacketData::copy() {
	return new GridOfflinePacketData(*this);
}
