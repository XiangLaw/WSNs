#include "elbar.h"
#include "../include/tcl.h"


static class ElbarGridOnlineHeaderClass : public PacketHeaderClass 
{
	public:
		ElbarGridOnlineHeaderClass() : PacketHeaderClass("PacketHeader/ELBARGRIDONLINE", sizeof(hdr_elbar_gridonline))
		{
			bind_offset(&hdr_elbar_gridonline::offset_);
		}
		~ElbarGridOnlineHeaderClass(){}
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

GridOnlineAgent::GridOnlineAgent() : GPSRAgent(),
		findStuck_timer_(this, &GridOnlineAgent::findStuckAngle),
		grid_timer_(this, &GridOnlineAgent::sendBoundHole)
{
	stuck_angle_ = NULL;
	hole_list_ = NULL;
	bind("range_", &range_);
	bind("limit_", &limit_);
	bind("r_", &r_);
	bind("limit_boundhole_hop_", &limit_boundhole_hop_);
}

