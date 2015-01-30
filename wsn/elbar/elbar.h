#ifndef _ELBAR_H_
#define _ELBAR_H_

#include <vector>
#include "../geomathhelper/geo_math_helper.h"
#include "../gridonline/gridonline.h"

struct angleView {
    struct polygonHole* hole_;
    double angle_;
};

struct parallelogram{
    struct polygonHole* hole;
    struct node* anchorLeft_;
    struct node* anchorRight_;
    struct node* anchorTop_;
};

class ElbarGridOnlineAgent: public GridOnlineAgent {
private:
    struct angleView*  angle_list_;
    struct parallelogram* parallelogram_list_;

    void routing();
    // detect covering parallelogram and view angle
    void detectParallelogram();
public:
	ElbarGridOnlineAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);
};

#endif
