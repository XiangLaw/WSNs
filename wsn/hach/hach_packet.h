#pragma once

#define HDR_HACH(p)    hdr_hach::access(p)

struct hdr_hach {
    Point cp_;

    inline int size() {
        return sizeof(Point);
    }

    static int offset_;

    inline static int &offset() {
        return offset_;
    }

    inline static struct hdr_hach *access(const Packet *p) {
        return (struct hdr_hach *) p->access(offset_);
    }
};
