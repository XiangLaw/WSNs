#include "common/ns-process.h"
#include "vhr_packet_data.h"

VHRPacketData::VHRPacketData() : AppData(VHR_DATA) {
    data_ = NULL;
    data_len_ = 0;
    element_size_ = sizeof(nsaddr_t) + 2 * sizeof(double) + sizeof(bool);
}

VHRPacketData::VHRPacketData(VHRPacketData &d) : AppData(d) {
    element_size_ = sizeof(nsaddr_t) + 2 * sizeof(double) + sizeof(bool);
    data_len_ = d.data_len_;

    if (data_len_ > 0) {
        data_ = new unsigned char[data_len_];
        memcpy(data_, d.data_, (size_t) data_len_);
    }
    else {
        data_ = NULL;
    }
}

void VHRPacketData::add(nsaddr_t id, double x, double y, bool is_convex_hull_boundary) {
    unsigned char *temp = data_;
    data_ = new unsigned char[data_len_ + element_size_];

    memcpy(data_, temp, (size_t) data_len_);
    memcpy(data_ + data_len_, &id, sizeof(nsaddr_t));
    memcpy(data_ + data_len_ + sizeof(nsaddr_t), &x, sizeof(double));
    memcpy(data_ + data_len_ + sizeof(nsaddr_t) + sizeof(double), &y, sizeof(double));
    memcpy(data_ + data_len_ + sizeof(nsaddr_t) + 2 * sizeof(double), &is_convex_hull_boundary, sizeof(bool));

    data_len_ += element_size_;
}

void VHRPacketData::dump() {
    FILE *fp = fopen("VHRDataDump.tr", "a+");

    for (int i = 1; i <= data_len_ / element_size_; i++) {
        BoundaryNode n = get_data(i);
        fprintf(fp, "%d\t%f\t%f\t%d\n", n.id_, n.x_, n.y_, n.is_convex_hull_boundary_);
    }

    fclose(fp);
}

BoundaryNode VHRPacketData::get_data(int index) {
    BoundaryNode re;
    int offset = (index - 1) * element_size_;

    memcpy(&re.id_, data_ + offset, sizeof(nsaddr_t));
    memcpy(&re.x_, data_ + offset + sizeof(nsaddr_t), sizeof(double));
    memcpy(&re.y_, data_ + offset + sizeof(nsaddr_t) + sizeof(double), sizeof(double));
    memcpy(&re.is_convex_hull_boundary_, data_ + offset + sizeof(nsaddr_t) + 2 * sizeof(double), sizeof(bool));

    return re;
}

int VHRPacketData::indexOf(BoundaryNode no) {
    return indexOf(no.id_, no.x_, no.y_);
}

int VHRPacketData::indexOf(nsaddr_t id, double x, double y) {
    BoundaryNode n;
    for (int i = 0; i < element_size_; i++) {
        n = get_data(i);
        if (n.id_ == id && n.x_ == x && n.y_ == y)
            return i;
    }

    return -1;
}

void VHRPacketData::rmv_data(int index) {
    if (index > data_len_ / element_size_ || index <= 0) return;

    int offset = (index - 1) * element_size_;

    unsigned char *temp = data_;
    data_ = new unsigned char[data_len_ - element_size_];

    memcpy(data_, temp, (size_t) offset);
    memcpy(data_ + offset, temp + offset + element_size_, (size_t) (data_len_ - offset - element_size_));

    data_len_ -= element_size_;
}

AppData *VHRPacketData::copy() {
    return new VHRPacketData(*this);
}

int VHRPacketData::size() const {
    return data_len_ / element_size_;
}