#ifndef _ELBAR_H_
#define _ELBAR_H_

#include <vector>
#include <math.h>
#include "../geomathhelper/geo_math_helper.h"
#include "../gpsr/gpsr.h"
#include "../gridonline/gridonline.h"


/*
* parallelogram P ABC
* */

struct parallelogram
{
    int hole_id_;
    Point* p_;
    Point* a_;
    Point* b_;
    Point* c_;
};

class ElbarGridOnlineAgent;

class ElbarGridOnlineAgent: public GridOnlineAgent {
public:
	ElbarGridOnlineAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);

protected:
	void routing();
    void holeCoveringParralelogramDetermination();

    virtual void sendBoundHole();
    virtual void recvBoundHole(Packet*);

private:
    double alpha_min_;
    double alpha_max_;
    double alpha_angle_;
    parallelogram* parallelogram_;
};

#endif
