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
void ElbarGridOnlineAgent::detectParallelogram(){
    struct polygonHole* hole;
    struct node* item;

    // detect view angle
    for (hole = hole_list_; hole != NULL; hole = hole->next_){
        item = hole->node_list_;
        struct node* left = hole->node_list_;
        struct node* right = hole->node_list_;
        if (item != NULL && item->next_ != NULL) {
            item = item->next_;
            do {
                Angle a = G::rawAngle(left, this, item, this);
                if (a < 0) left = item;
                a = G::rawAngle(right, this, item, this);
                if (a > 0) right = item;
                item = item->next_;
            } while (item && item != hole->node_list_);
        }
    }
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
