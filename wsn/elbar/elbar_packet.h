#ifndef ELBAR_GRIDONLINE_H_
#define ELBAR_GRIDONLINE_H_

#include "packet.h"

#define HDR_ELBAR_GRID(p) hdr_elbar_gridonline::access(p)

struct hdr_elbar_gridonline
{
    Point last_;	// Pre-previews node
    Point prev_;	// Previews node
    Point i_;

    inline int size() { return 3 * sizeof(Point); }

    static int offset_;
    inline static int& offset() { return offset_; }
    inline static struct hdr_elbar_gridonline* access(const Packet *p)
    {
        return (struct hdr_elbar_gridonline*)p->access(offset_);
    }
};

#endif /* ELBAR_GRIDONLINE_H_ */
