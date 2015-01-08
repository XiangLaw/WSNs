/*
 * goal.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */

#include "../include/tcl.h"
#include "wsn/boundhole/boundhole_packet_data.h"
#include "goal.h"

static class GoalAgentClass : public TclClass
{
	public:
		GoalAgentClass() : TclClass("Agent/GOAL") {}
		TclObject* create(int, const char*const*)
		{
			return (new GoalAgent());
		}
}class_goal;

// --------------------------- Agent ------------------------------ //

GoalAgent::GoalAgent() : ConvexHullAgent()
{
	lastTTL_ = -1;
	bind("broadcast_hop_", 	&broadcast_hop_);
	bind("limit_", & limit_);
}

// --------------------- Broadcast hole identifier ---------------- //

//void GoalAgent::sendBCH(Packet* h)
//{
//	// ----------------- header
//	Packet*	p = allocpkt();
//	struct hdr_cmn*	cmh = HDR_CMN(p);
//	struct hdr_ip*	iph = HDR_IP(p);
//
//	cmh->ptype() 	 = PT_CONVEXHULL;
//	cmh->next_hop_	 = IP_BROADCAST;
//	cmh->last_hop_ 	 = my_id_;
//	cmh->addr_type_  = NS_AF_INET;
//	cmh->size()		 = IP_HDR_LEN;
//
//	iph->daddr() = IP_BROADCAST;
//	iph->saddr() = my_id_;
//	iph->sport() = RT_PORT;
//	iph->dport() = RT_PORT;
//	iph->ttl_	 = broadcast_hop_;
//
//	// ------------------ data
//	BoundHolePacketData *data = (BoundHolePacketData*)h->userdata();
//	p->setdata(data);
//	send(p, 0);
//}

void GoalAgent::sendBCH(polygonHole* h)
{
	// ----------------- header
	Packet*	p = allocpkt();
	struct hdr_cmn*	cmh = HDR_CMN(p);
	struct hdr_ip*	iph = HDR_IP(p);

	cmh->ptype() 	 = PT_CONVEXHULL;
	cmh->next_hop_	 = IP_BROADCAST;
	cmh->last_hop_ 	 = my_id_;
	cmh->addr_type_  = NS_AF_INET;
	cmh->size()		 = IP_HDR_LEN;

	iph->daddr() = IP_BROADCAST;
	iph->saddr() = my_id_;
	iph->sport() = RT_PORT;
	iph->dport() = RT_PORT;
	iph->ttl_	 = broadcast_hop_ > 0 ? broadcast_hop_ : 10000;

	// ------------------ data
	BoundHolePacketData* data = new BoundHolePacketData();
	node* i = h->node_list_;
	do {
		data->add(h->hole_id_, i->x_, i->y_);
		i = i->next_;
	} while(i != h->node_list_);
	p->setdata(data);
	send(p, 0);
}

void GoalAgent::recvBCH(Packet* p)
{
	struct hdr_cmn*		cmh = HDR_CMN(p);
	struct hdr_ip*		iph = HDR_IP(p);

	// check if is really receive this hole's information
	for (polygonHole* h = hole_list_; h; h = h->next_)
	{
		if (h->hole_id_ == iph->saddr() && iph->ttl() <= lastTTL_)	// already received
		{
			drop(p, "Received");
			return;
		}
	}

	// dump
	if (lastTTL_ == -1)	// first time receive BCH
	{
		dumpBroadcast();
	}

	// update lastTTL_
	lastTTL_ = iph->ttl();

	// create hole item
	polygonHole* newHole = createPolygonHole(p);
	newHole->hole_id_	 = iph->saddr();
	newHole->next_		 = hole_list_;
	hole_list_			 = newHole;

	// Check if this node is boundary node
	node* n;
	for (n = newHole->node_list_; n; n = n->next_)
	{
		if (*n == *this)
		{
			iph->ttl_ = broadcast_hop_ + 1;
		}
	}

	// cycle the node_list
	n = newHole->node_list_;
	while (n->next_) n = n->next_;
	n->next_ = newHole->node_list_;

	// broadcast message
	cmh->direction_ = hdr_cmn::DOWN;
	cmh->last_hop_  = my_id_;

	iph->ttl_--;
	if (iph->ttl_ <= 0)	drop(p, "Limit");
	else				send(p, 0);
}

// ------------------------ Dump ------------------------ //

void GoalAgent::dumpBroadcast()
{
	FILE *fp = fopen("BroadcastRegion.tr", "a+");

	fprintf(fp, "%f	%f\n", this->x_, this->y_);

	fclose(fp);
}

// ------------------------ End ------------------------ //
