#include <tclDecls.h>
#include <bits/ios_base.h>
#include "elbar.h"
#include "elbar_packet.h"
#include "../include/tcl.h"

int hdr_elbar_gridonline::offset_;

static class ElbarGridOnlineHeaderClass : public PacketHeaderClass 
{
	public:
		ElbarGridOnlineHeaderClass() : PacketHeaderClass("PacketHeader/ELBARGRIDONLINE", sizeof(hdr_elbar_gridonline))
		{
			bind_offset(&hdr_elbar_gridonline::offset_);
		}
} class_elbargridonlinehdr;

static class ElbarGridOnlineAgentClass : public TclClass
{
	public:
		ElbarGridOnlineAgentClass() : TclClass("Agent/ELBARGRIDONLINE") {}
		TclObject *create(int argc, const char*const* argv) 
		{
			return (new ElbarGridOnlineAgent());
		}
} class_elbargridonline;

/**
 * Agent implementation
 */

ElbarGridOnlineAgent::ElbarGridOnlineAgent() : GridOnlineAgent()
{
}

int
ElbarGridOnlineAgent::command(int argc, const char*const* argv)
{
    if (argc == 2){
        if (strcasecmp(argv[1], "routing") == 0){
            // routing use elbar algorithm
            routing();
            return TCL_OK;
        }
    }

    return GridOnlineAgent::command(argc,argv);
}

// handle the receive packet just of type PT_ELBARGRIDONLINE
void
ElbarGridOnlineAgent::recv(Packet *p, Handler *h)
{
    hdr_cmn *cmh = HDR_CMN(p);

    switch (cmh->ptype())
    {
        case PT_HELLO:
            GPSRAgent::recv(p, h);
            break;

        case PT_ELBARGRIDONLINE:
            recvBoundHole(p);
            break;

        default:
            drop(p, " UnknowType");
            break;
    }
}

/*------------------------------ Routing --------------------------*/
/*
 * Hole covering parallelogram determination
 */
bool ElbarGridOnlineAgent::detectParallelogram(){
    struct polygonHole*tmp;
    struct node* item;

    struct node* ai;
    struct node* aj;

    struct node a;
    struct node b;
    struct node c;

    struct node* vi;
    struct node* vj;

    double hi;
    double hj;
    double h;

    Line li;
    Line lj;

    /*// detect view angle
    for (tmp = hole_list_; tmp != NULL; tmp = tmp->next_){
        item = tmp->node_list_;
        struct node* left = tmp->node_list_;
        struct node* right = tmp->node_list_;
        if (item != NULL && item->next_ != NULL) {
            item = item->next_;
            do {
                Angle a = G::rawAngle(left, this, item, this);
                if (a < 0) left = item;
                a = G::rawAngle(right, this, item, this);
                if (a > 0) right = item;
                item = item->next_;
            } while (item && item != tmp->node_list_);
        }
    }*/

    // detect view angle
    ai = hole_list_->node_list_;
    aj = hole_list_->node_list_;
    item = tmp->node_list_;
    for(tmp = hole_list_; tmp != NULL; tmp = tmp->next_)
    {
        if(G::directedAngle(ai, this, item) > 0)
            ai = item;
        if(G::directedAngle(aj, this, item) < 0)
            aj = item;
        item = item->next_;
    }

    // detect parallelogram
    vi = hole_list_->node_list_;
    vj = hole_list_->node_list_;
    hi = 0;
    hj = 0;
    item = tmp->node_list_; // A(k)

    for(tmp = hole_list_; tmp != NULL; tmp = tmp->next_)
    {
        h = G::distance(item->x_, item->y_, this->x_, this->y_, ai->x_, ai->y_);
        if(h > hi) {
            hi = h;
            vi = item;
        }
        h = G::distance(item->x_, item->y_, this->x_, this->y_, aj->x_, aj->y_);
        if(h > hj) {
            hj = h;
            vj = item;
        }
        item = item->next_;
    }

    li = G::parallel_line(vi, G::line(this, ai));
    lj = G::parallel_line(vj, G::line(this, aj));

    if(!G::intersection(lj, G::line(this, ai), a) ||
        !G::intersection(li, G::line(this, aj), c) ||
        !G::intersection(li, lj, b))
        return false;

    this->parallelogram_->a_ = a;
    this->parallelogram_->b_ = b;
    this->parallelogram_->c_ = c;
    this->parallelogram_->p_.x_ =this->x_;
    this->parallelogram_->p_.y_ = this->y_;

    return true;
}

/*
 * Hole bypass routing
 */
void ElbarGridOnlineAgent::routing() {

}

/*
 * recv packet
 */
void GridOnlineAgent::recvBoundHole(Packet *) {
}

/*
 * sendBoudHole info
 */
