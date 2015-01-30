/*
*
* */

#include "common/ns-process.h"
#include "elbar_packet_data.h"
#include "cstring"

ElbarGridOnlinePacketData::ElbarGridOnlinePacketData() : AppData(ELBAR_GRIDONLINE_DATA)
{
    data_ = NULL;
    data_len_ = 0;
    element_size_ = 2 * sizeof(double);
}

ElbarGridOnlinePacketData::ElbarGridOnlinePacketData(ElbarGridOnlinePacketData &d) : AppData(d)
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

void ElbarGridOnlinePacketData::addData(Point p)
{
    unsigned char* temp = data_;
    data_ = new unsigned char[data_len_ + element_size_];

    memcpy(data_, temp, data_len_);
    memcpy(data_ + data_len_, &(p.x_), sizeof(double));
    memcpy(data_ + data_len_ + sizeof(double), &(p.y_), sizeof(double));

    data_len_ += element_size_;
}

void
ElbarGridOnlinePacketData::addData(int index, Point p)
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

void ElbarGridOnlinePacketData::dump()
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

Point ElbarGridOnlinePacketData::getData(int index)
{
    Point re;
    int offset = index * element_size_;

    memcpy(&re.x_,  data_ + offset,  sizeof(double));
    memcpy(&re.y_,  data_ + offset + sizeof(double), sizeof(double));

    return re;
}

void ElbarGridOnlinePacketData::removeData(int index)
{
    if (index >= size() || index < 0) return;

    int offset = index * element_size_;

    unsigned char * temp = data_;
    data_ = new unsigned char [data_len_ - element_size_];

    memcpy(data_, temp, offset);
    memcpy(data_ + offset, temp + offset + element_size_, data_len_ - offset - element_size_);

    data_len_ -= element_size_;
}

AppData* ElbarGridOnlinePacketData::copy()
{
    return new ElbarGridOnlinePacketData(*this);
}

int ElbarGridOnlinePacketData::size() const
{
    return data_len_ / element_size_;
}
