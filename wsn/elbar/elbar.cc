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
    alpha_min_ = M_PI / 3;
    alpha_max_ = 2 * M_PI / 3;
    parallelogram_ = NULL;
}

int
ElbarGridOnlineAgent::command(int argc, const char*const* argv)
{
    return GridOnlineAgent::command(argc,argv);
}

// handle the receive packet just of type PT_GRID
void
ElbarGridOnlineAgent::recv(Packet *p, Handler *h)
{
    hdr_cmn *cmh = HDR_CMN(p);

    switch (cmh->ptype())
    {
        case PT_HELLO:
            GridOnlineAgent::recv(p, h);
            break;

        case PT_ELBARGRIDONLINE:
            // TODO: handle here
            break;

        default:
            drop(p, " UnknowType");
            break;
    }
}

void ElbarGridOnlineAgent::holeCoveringParralelogramDetermination() {

}

void ElbarGridOnlineAgent::routing() {

}

void ElbarGridOnlineAgent::sendBoundHole() {
    GridOnlineAgent::sendBoundHole();
}

void ElbarGridOnlineAgent::recvBoundHole(Packet *packet) {
    GridOnlineAgent::recvBoundHole(packet);
}
