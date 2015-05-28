#include "griddynamic_packet_data.h"

GridDynamicPacketData::GridDynamicPacketData() : AppData(GRIDOFFLINE_DATA)
{
    data_ = NULL;
    nx_ = 0;
    ny_ = 0;
}

GridDynamicPacketData::GridDynamicPacketData(GridDynamicPacketData &d) : AppData(d)
{
    nx_ = d.nx_;
    ny_ = d.ny_;

    if (nx_ * ny_ > 0)
    {
        data_ = new bool[nx_ * ny_];
        memcpy(data_, d.data_, nx_ * ny_);
    }
    else
    {
        data_ = NULL;
    }
}

void GridDynamicPacketData::addData(bool** arr, int x_, int y_)
{
    nx_ = x_;
    ny_ = y_;
    data_ = new bool[nx_ * ny_];

    memcpy(data_, arr, nx_ * ny_);
}

bool** GridDynamicPacketData::getData()
{
    bool** re = new bool*[nx_];
    for(int i = 0; i < nx_; i++){
        re[i] = new bool[ny_];
        memcpy(&(re[i*ny_]), data_ + i*ny_,  ny_);
    }

    return re;
}

int GridDynamicPacketData::size() const {
    return nx_ * ny_;
}

void GridDynamicPacketData::dump()
{
}

AppData* GridDynamicPacketData::copy() {
    return new GridDynamicPacketData(*this);
}
