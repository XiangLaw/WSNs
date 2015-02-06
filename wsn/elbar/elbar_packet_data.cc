#include "common/ns-process.h"
#include "elbar_packet_data.h"
#include "cstring"

ElbarGridOfflinePacketData::ElbarGridOfflinePacketData() : AppData(ELBAR_GRIDOFFLINE_DATA) {
    data_ = NULL;
    data_len_ = 0;
    element_size_ = 2 * sizeof(double);
}

ElbarGridOfflinePacketData::ElbarGridOfflinePacketData(ElbarGridOfflinePacketData &d) : AppData(d) {
    element_size_ = 2 * sizeof(double);
    data_len_ = d.data_len_;

    if (data_len_ > 0) {
        data_ = new unsigned char[data_len_];
        memcpy(data_, d.data_, data_len_);
    }
    else {
        data_ = NULL;
    }
}

void ElbarGridOfflinePacketData::add_data(double x, double y) {
    unsigned char *temp = data_;
    data_ = new unsigned char[data_len_ + element_size_];

    memcpy(data_, temp, data_len_);
    memcpy(data_ + data_len_, &x, sizeof(double));
    memcpy(data_ + data_len_ + sizeof(double), &y, sizeof(double));

    data_len_ += element_size_;
}

void ElbarGridOfflinePacketData::dump() {
    FILE *fp = fopen("ElbarGridOnline.tr", "a+");

    for (int i = 1; i <= data_len_ / element_size_; i++) {
        node n = get_data(i);
        fprintf(fp, "%f\t%f\n", n.x_, n.y_);
    }
    node n = get_data(1);
    fprintf(fp, "%f\t%f\n", n.x_, n.y_);
    fprintf(fp, "\n");
    fclose(fp);
}

node ElbarGridOfflinePacketData::get_data(int index) {
    node re;
    int offset = (index - 1) * element_size_;
    memcpy(&re.x_, data_ + offset, sizeof(double));
    memcpy(&re.y_, data_ + offset + sizeof(double), sizeof(double));

    return re;
}

int ElbarGridOfflinePacketData::indexOf(node no) {
    return indexOf(no.x_, no.y_);
}

int ElbarGridOfflinePacketData::indexOf(double x, double y) {
    node n;
    for (int i = 0; i < element_size_; i++) {
        n = get_data(i);
        if (n.x_ == x && n.y_ == y)
            return i;
    }

    return -1;
}

void ElbarGridOfflinePacketData::rmv_data(int index) {
    if (index > data_len_ / element_size_ || index <= 0) return;

    int offset = (index - 1) * element_size_;

    unsigned char *temp = data_;
    data_ = new unsigned char[data_len_ - element_size_];

    memcpy(data_, temp, offset);
    memcpy(data_ + offset, temp + offset + element_size_, data_len_ - offset - element_size_);

    data_len_ -= element_size_;
}

AppData *ElbarGridOfflinePacketData::copy() {
    return new ElbarGridOfflinePacketData(*this);
}

int ElbarGridOfflinePacketData::size() const {
    return data_len_ / element_size_;
}
