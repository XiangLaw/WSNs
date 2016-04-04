#include "corbal_packet_data.h"

CorbalPacketData::CorbalPacketData() : AppData(CORBAL_DATA)
{
    data_ = NULL;
    data_len_ = 0;
    element_size_ = sizeof(nsaddr_t) + 2 * sizeof(double);
}

CorbalPacketData::CorbalPacketData(CorbalPacketData &d) : AppData(d)
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

void CorbalPacketData::add(nsaddr_t id, double x, double y)
{
    unsigned char* temp = data_;
    data_ = new unsigned char[data_len_ + element_size_];

    memcpy(data_, temp, data_len_);
    memcpy(data_ + data_len_, &id, sizeof(nsaddr_t));
    memcpy(data_ + data_len_ + sizeof(nsaddr_t), &x, sizeof(double));
    memcpy(data_ + data_len_ + sizeof(nsaddr_t) + sizeof(double), &y, sizeof(double));

    data_len_ += element_size_;
}

// note: never call ADD method after calling this method. it means this method must to be called after all
// Bi node store schema: next_index_of_1, B(1-1), B(1-2), ..., B(1-n), next_index_of_2, B(2-1), ..., B(2-n), ...
void CorbalPacketData::addHBA(int n, int kn) {
    unsigned char *tmp = data_;
    data_ = new unsigned char[data_len_ + (n + 1) * kn * element_size_](); // initialize all element to zero
    memcpy(data_, tmp, data_len_);
    data_len_ += (n + 1) * kn * element_size_;
}


void CorbalPacketData::addBiNode(int off, nsaddr_t id, double x, double y)
{
    int offset = off * element_size_;
    memcpy(data_ + offset, &id, sizeof(nsaddr_t));
    memcpy(data_ + offset + sizeof(nsaddr_t), &x, sizeof(double));
    memcpy(data_ + offset + sizeof(nsaddr_t) + sizeof(double), &y, sizeof(double));
}

void CorbalPacketData::dump() {
    FILE *fp = fopen("BoundHole.tr", "a+");

    for (int i = 1; i <= data_len_ / element_size_; i++)
    {
        node n = get_data(i);
        fprintf(fp, "%d\t%f\t%f\n", n.id_, n.x_, n.y_);
    }

    fclose(fp);
}

node CorbalPacketData::get_data(int index)
{
    node re;
    int offset = (index - 1) * element_size_;

    memcpy(&re.id_, data_ + offset,  sizeof(nsaddr_t));
    memcpy(&re.x_,  data_ + offset + sizeof(nsaddr_t),  sizeof(double));
    memcpy(&re.y_,  data_ + offset + sizeof(nsaddr_t) + sizeof(double), sizeof(double));

    return re;
}


int CorbalPacketData::indexOf(node no)
{
    return indexOf(no.id_, no.x_, no.y_);
}

int CorbalPacketData::indexOf(nsaddr_t id, double x, double y)
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

void CorbalPacketData::rmv_data(int index)
{
    if (index > data_len_ / element_size_ || index <= 0) return;

    int offset = (index - 1) * element_size_;

    unsigned char * temp = data_;
    data_ = new unsigned char [data_len_ - element_size_];

    memcpy(data_, temp, offset);
    memcpy(data_ + offset, temp + offset + element_size_, data_len_ - offset - element_size_);

    data_len_ -= element_size_;
}

AppData* CorbalPacketData::copy()
{
    return new CorbalPacketData(*this);
}

int CorbalPacketData::size() const
{
    return data_len_ / element_size_;
}

node CorbalPacketData::get_Bi_data(int off)
{
    node re;
    int offset = off * element_size_;

    memcpy(&re.id_, data_ + offset,  sizeof(nsaddr_t));
    memcpy(&re.x_,  data_ + offset + sizeof(nsaddr_t),  sizeof(double));
    memcpy(&re.y_,  data_ + offset + sizeof(nsaddr_t) + sizeof(double), sizeof(double));

    return re;
}

int CorbalPacketData::get_next_index_of_Bi(int off)
{
    int re = 0;
    int offset = off * element_size_;
    memcpy(&re, data_ + offset, sizeof(int));

    return re;
}

void CorbalPacketData::update_next_index_of_Bi(int off, int next_index) {
    int offset = off * element_size_;
    memcpy(data_ + offset, &next_index, sizeof(int));
}
