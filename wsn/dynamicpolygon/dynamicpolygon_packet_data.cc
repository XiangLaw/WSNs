/*
 * dynamicpolygon_packet_data.cc
 *
 * Created on: Mar 27, 2014
 * author :    trongnguyen
 */

#include "common/ns-process.h"
#include "dynamicpolygon_packet_data.h"
#include "cstring"

DynamicPolygonPacketData::DynamicPolygonPacketData() : AppData(DYNAMICPOLYGON_DATA)
{
	data_ = NULL;
	data_len_ = 0;
	element_size_ = 2 * sizeof(double);
}

DynamicPolygonPacketData::DynamicPolygonPacketData(DynamicPolygonPacketData &d) : AppData(d)
{
	element_size_ = 2 * sizeof(double);
	data_len_ = d.data_len_;

	if (data_len_ > 0)
	{
		data_ = new unsigned char[data_len_];
		memcpy(data_, d.data_, data_len_);
	}
	else
	{
		data_ = NULL;
	}
}

void DynamicPolygonPacketData::addData(Point p)
{
	unsigned char* temp = data_;
	data_ = new unsigned char[data_len_ + element_size_];

	memcpy(data_, temp, data_len_);
	memcpy(data_ + data_len_, &(p.x_), sizeof(double));
	memcpy(data_ + data_len_ + sizeof(double), &(p.y_), sizeof(double));

	data_len_ += element_size_;
}

void
DynamicPolygonPacketData::addData(int index, Point p)
{
	unsigned char* temp = data_;
	data_ = new unsigned char[data_len_ + element_size_];

	int offset = index * element_size_;

	memcpy(data_, temp, offset);
	memcpy(data_ + offset, &p.x_, sizeof(double));
	memcpy(data_ + offset + sizeof(double), &p.y_, sizeof(double));
	memcpy(data_ + offset + sizeof(double) + sizeof(double), temp + offset, data_len_ - offset);

	data_len_ += element_size_;
}

void DynamicPolygonPacketData::dump()
{
	FILE *fp = fopen("DataDump.tr", "w");

	for (int i = 0; i < size(); i++)
	{
		Point n = getData(i);
		fprintf(fp, "%f\t%f\n", n.x_, n.y_);
	}
	fprintf(fp, "\n");
	fclose(fp);
}

Point DynamicPolygonPacketData::getData(int index)
{
	Point re;
	int offset = index * element_size_;

	memcpy(&re.x_,  data_ + offset,  sizeof(double));
	memcpy(&re.y_,  data_ + offset + sizeof(double), sizeof(double));

	return re;
}

void DynamicPolygonPacketData::removeData(int index)
{
	if (index >= size() || index < 0) return;

	int offset = index * element_size_;

	unsigned char * temp = data_;
	data_ = new unsigned char [data_len_ - element_size_];

	memcpy(data_, temp, offset);
	memcpy(data_ + offset, temp + offset + element_size_, data_len_ - offset - element_size_);

	data_len_ -= element_size_;
}

void DynamicPolygonPacketData::removeAllData() {
    data_ = NULL;
    data_len_ = 0;
}

AppData* DynamicPolygonPacketData::copy()
{
	return new DynamicPolygonPacketData(*this);
}

int DynamicPolygonPacketData::size() const
{
	return data_len_ / element_size_;
}
