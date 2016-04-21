#include "common/ns-process.h"
#include "hach_packet_data.h"
#include "cstring"

HACHPacketData::HACHPacketData() : AppData(HACH_DATA)
{
    data_ = NULL;
    data_len_ = 0;
    element_size_ = sizeof(nsaddr_t) + 2 * sizeof(double);
}

HACHPacketData::HACHPacketData(HACHPacketData &d) : AppData(d)
{
    element_size_ = sizeof(nsaddr_t) + 2 * sizeof(double);
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

void HACHPacketData::add(nsaddr_t id, double x, double y)
{
    unsigned char* temp = data_;
    data_ = new unsigned char[data_len_ + element_size_];

    memcpy(data_, temp, data_len_);
    memcpy(data_ + data_len_, &id, sizeof(nsaddr_t));
    memcpy(data_ + data_len_ + sizeof(nsaddr_t), &x, sizeof(double));
    memcpy(data_ + data_len_ + sizeof(nsaddr_t) + sizeof(double), &y, sizeof(double));

    data_len_ += element_size_;
}

void HACHPacketData::dump() {
    FILE *fp = fopen("BCPCoverage.tr", "a+");

    for (int i = 0; i < data_len_ / element_size_; i++)
    {
        node n = get_data(i);
        fprintf(fp, "%d\t%f\t%f\n", n.id_, n.x_, n.y_);
    }
    fprintf(fp, "\n");

    fclose(fp);
}

node HACHPacketData::get_data(int index)
{
    node re;
    int offset = index * element_size_;

    memcpy(&re.id_, data_ + offset,  sizeof(nsaddr_t));
    memcpy(&re.x_,  data_ + offset + sizeof(nsaddr_t),  sizeof(double));
    memcpy(&re.y_,  data_ + offset + sizeof(nsaddr_t) + sizeof(double), sizeof(double));

    return re;
}

int HACHPacketData::indexOf(node no)
{
    return indexOf(no.id_, no.x_, no.y_);
}

int HACHPacketData::indexOf(nsaddr_t id, double x, double y)
{
    node n;
    for (int i = 0; i < element_size_; i++)
    {
        n = get_data(i);
        if (n.id_ == id && n.x_ == x && n.y_ == y)
            return i;
    }

    return -1;
}

void HACHPacketData::rmv_data(int index)
{
    if (index >= data_len_ / element_size_ || index < 0) return;

    int offset = index * element_size_;

    unsigned char * temp = data_;
    data_ = new unsigned char [data_len_ - element_size_];

    memcpy(data_, temp, offset);
    memcpy(data_ + offset, temp + offset + element_size_, data_len_ - offset - element_size_);

    data_len_ -= element_size_;
}

AppData* HACHPacketData::copy()
{
    return new HACHPacketData(*this);
}

int HACHPacketData::size() const
{
    return data_len_ / element_size_;
}
