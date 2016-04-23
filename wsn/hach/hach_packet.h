#pragma once

#define HACH_BOUNDHOLE  1
#define HACH_HACH       2

#define HDR_HACH(p)    hdr_hach::access(p)

struct hdr_hach {
    Point cp_;
    uint8_t type_;

    inline int size() {
        return sizeof(Point) + sizeof(uint8_t);
    }

    static int offset_;

    inline static int &offset() {
        return offset_;
    }

    inline static struct hdr_hach *access(const Packet *p) {
        return (struct hdr_hach *) p->access(offset_);
    }
};
