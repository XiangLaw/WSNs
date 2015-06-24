/*
 * ehds.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */

#include "../include/tcl.h"
#include "ehds.h"

int hdr_ehds::offset_;

static class EHDSHeaderClass : public PacketHeaderClass {
public:
	EHDSHeaderClass() : PacketHeaderClass("PacketHeader/EHDS", sizeof(hdr_all_ehds))
	{
		bind_offset(&hdr_ehds::offset_);
	}
} class_ehdshdr;

static class EHDSAgentClass : public TclClass {
public:
	EHDSAgentClass() : TclClass("Agent/EHDS") {}
	TclObject* create(int, const char*const*) {
		return (new EHDSAgent());
	}
}class_ehds;

// --------------------- Agent ------------------------------------ //

EHDSAgent::EHDSAgent() : BoundHoleAgent()
{
	circleHole_list_ = NULL;
	routing_num_ = 0;

	// clear trace file
	FILE * fp;
	fp = fopen("ApproximateHole.tr", "w");	fclose(fp);
	fp = fopen("RoutingTable.tr", "w");		fclose(fp);
	fp = fopen("Hopcount.tr", "w");			fclose(fp);
	fp = fopen("BroadcastRegion.tr", "w");	fclose(fp);
}

void
EHDSAgent::recv(Packet* p, Handler* h)
{
	struct hdr_cmn* cmh = HDR_CMN(p);

	if (cmh->ptype() == PT_EHDS)
		recvEHDS(p);
	else
		BoundHoleAgent::recv(p, h);
}

int
EHDSAgent::command(int argc, const char*const* argv)
{
	if (argc == 2)
	{
		if (strcasecmp(argv[1], "routing") == 0)
		{
			routing();
			return TCL_OK;
		}
		if (strcasecmp(argv[1], "dump") == 0)
		{
			dumpRoutingTable();
		}
	}

	return BoundHoleAgent::command(argc, argv);
}

// --------------------- Approximate hole ------------------------- //

void
EHDSAgent::createHole(Packet* p)
{
	polygonHole* h = createPolygonHole(p);

	// create new circleHole
	circleHole* newHole = new circleHole();
	newHole->hole_id_	= h->hole_id_;
	newHole->next_		= circleHole_list_;
	circleHole_list_	= newHole;

	// approximate hole to circleHole
	double minx = 1000;
	double miny = 1000;
	double maxx = 0;
	double maxy = 0;
	for (struct node* ntemp = h->node_list_; ntemp; ntemp = ntemp->next_)
	{
		minx = minx < ntemp->x_ ? minx : ntemp->x_;
		miny = miny < ntemp->y_ ? miny : ntemp->y_;
		maxx = maxx > ntemp->x_ ? maxx : ntemp->x_;
		maxy = maxy > ntemp->y_ ? maxy : ntemp->y_;
	}
	newHole->x_ = (minx + maxx) / 2;
	newHole->y_ = (miny + maxy) / 2;

	// calculate radius
	newHole->radius_ = 0;
	for (node* ntemp = h->node_list_; ntemp; ntemp = ntemp->next_)
	{
		double d = G::distance(newHole, ntemp);
		newHole->radius_ = newHole->radius_ > d ? newHole->radius_ : d;
	}

	delete h;

	// routing
	routing();

	// broadcast hole information
	if (storage_opt_ == STORAGE_ONE)
	{
		sendBC();
	}

	// dump
	dumpApproximateHole();
}

// --------------------- Broadcast Hole identifier ---------------- //

void
EHDSAgent::sendHA(nsaddr_t saddr, Point spos)
{
	Packet*	p = allocpkt();
	struct hdr_cmn*			cmh = HDR_CMN(p);
	struct hdr_ip*			iph = HDR_IP(p);
	struct hdr_ehds_ha*	ehh = HDR_EHDS_HA(p);

	cmh->ptype() = PT_EHDS;
	cmh->size()	 = IP_HDR_LEN + ehh->size();

	iph->daddr() = saddr;
	iph->saddr() = my_id_;
	iph->sport() = RT_PORT;
	iph->dport() = RT_PORT;
	iph->ttl_	 = IP_DEF_TTL;

	ehh->type_ 				= EHDS_HA;
	ehh->id_	 			= circleHole_list_->hole_id_;
	ehh->circle_.x_ 		= circleHole_list_->x_;
	ehh->circle_.y_ 		= circleHole_list_->y_;
	ehh->circle_.radius_ 	= circleHole_list_->radius_;
	ehh->spos_				= spos;

	recvEHDS(p);
}

void
EHDSAgent::sendBC()
{
	Packet *p = allocpkt();
	struct hdr_cmn 		*cmh = HDR_CMN(p);
	struct hdr_ip 		*iph = HDR_IP(p);
	struct hdr_ehds_ha	*ehh = HDR_EHDS_HA(p);

	cmh->ptype() 	= PT_EHDS;
	cmh->next_hop_  = IP_BROADCAST;
	cmh->last_hop_  = my_id_;
	cmh->addr_type_ = NS_AF_INET;
	cmh->size() 	= IP_HDR_LEN + ehh->size();

	iph->daddr() = IP_BROADCAST;
	iph->saddr() = my_id_;
	iph->sport() = RT_PORT;
	iph->dport() = RT_PORT;
	iph->ttl_	 = IP_DEF_TTL;

	ehh->type_ 				= EHDS_BC;
	ehh->id_	 			= circleHole_list_->hole_id_;
	ehh->circle_.x_ 		= circleHole_list_->x_;
	ehh->circle_.y_ 		= circleHole_list_->y_;
	ehh->circle_.radius_ 	= circleHole_list_->radius_;

	//send(p, 0);
	recvEHDS(p);
}

void
EHDSAgent::recvEHDS(Packet* p)
{
	hdr_cmn*		cmh = HDR_CMN(p);
	hdr_ip*			iph = HDR_IP(p);
	hdr_ehds_ha*	edh = HDR_EHDS_HA(p);

	// check if is really receive this hole's information
	bool isReceived = false;
	for (circleHole* hole = circleHole_list_; hole && !isReceived; hole = hole->next_)
	{
		isReceived = hole->hole_id_ == edh->id_;
	}
	if (!isReceived)
	{
		// add new circle hole
		circleHole* newHole = (circleHole*)malloc(sizeof(circleHole));
		newHole->hole_id_	= edh->id_;
		newHole->radius_ 	= edh->circle_.radius_;
		newHole->x_			= edh->circle_.x_;
		newHole->y_			= edh->circle_.y_;
		newHole->next_		= circleHole_list_;
		circleHole_list_	= newHole;

		routing();
	}

	// forward packet
	if (edh->type_ == EHDS_HA)
	{
		dumpBroadcastRegion();

		if (cmh->direction() == hdr_cmn::UP	&& iph->daddr() == my_id_)	// up to destination
		{
			drop(p, " Finish");
		}
		else
		{
			// -------- forward by greedy
			node * nexthop = getNeighborByGreedy(edh->spos_);

			if (nexthop == NULL)	// no neighbor close
			{
				drop(p, DROP_RTR_NO_ROUTE);
				return;
			}
			else
			{
				cmh->direction() = hdr_cmn::DOWN;
				cmh->addr_type() = NS_AF_INET;
				cmh->last_hop_ = my_id_;
				cmh->next_hop_ = nexthop->id_;
				send(p, 0);
			}
		}
	}
	else	// edh->type == EHDS_BC
	{
		if (isReceived)
		{
			drop(p, DROP_RTR_ROUTE_LOOP);
		}
		else
		{
			// continue broadcast packet
			cmh->direction() = hdr_cmn::DOWN;
			cmh->addr_type() = NS_AF_INET;
			cmh->last_hop_   = my_id_;
			cmh->next_hop_ 	 = IP_BROADCAST;
			send(p, 0);
		}
	}
}

// --------------------- Routing ---------------------------------- //

void
EHDSAgent::routing()
{
	// clear routing table
	routing_num_ = 0;

	if (dest->x_ || dest->y_)
	{
		// Add destination to routing table
		addrouting(dest);

		// create routing table
		Line SD = G::line(this, dest);

		for (circleHole* h = circleHole_list_; h; h = h->next_)
		{
			// check if need to routing to avoid this hole
			if (G::distance(h, SD) < h->radius_ && G::distance(this, h) >= h->radius_ && G::distance(dest, h) >= h->radius_)
			{
				// calculate temp destination
				Point s1, s2, d1, d2;

				G::tangent_point(h, this, s1, s2);
				G::tangent_point(h, dest, d1, d2);

				Line ts = G::is_intersect(this, dest, h, &s1) ? G::line(this, s1) : G::line(this, s2);
				Line td = G::is_intersect(this, dest, h, &d1) ? G::line(dest,  d1) : G::line(dest,  d2);

				Point in;
				if (G::intersection(ts, td, in))
					addrouting(in);

			} // if (d < r)
		} // for each hole
	} // if (dest->x_ || dest->y_)
	// Add source to routing table
	addrouting(this);
	routing_num_--;			// go to myself, remove "this" from routing table
}

void
EHDSAgent::addrouting(Point* p)
{
	for (int i = 0; i < routing_num_; i++)
	{
		if (routing_table[i].x_ == p->x_ && routing_table[i].y_ == p->y_)
			return;
	}

	routing_table[routing_num_].x_ = p->x_;
	routing_table[routing_num_].y_ = p->y_;
	routing_num_++;
}

// --------------------- Send data -------------------------------- //

void
EHDSAgent::sendData(Packet* p)
{
	hdr_cmn*			cmh = HDR_CMN(p);
	hdr_ip*				iph = HDR_IP(p);
	hdr_ehds_data* 	edh = HDR_EHDS_DATA(p);

	cmh->size() += IP_HDR_LEN + edh->size();
	cmh->direction_ = hdr_cmn::DOWN;

	edh->type_ 			= routing_num_ > 2 ? EHDS_DATA_ROUTING : EHDS_DATA_GREEDY;
	edh->daddr_ 		= iph->daddr();
	edh->vertex_num_	= routing_num_;
	for (int i = 0; i <= routing_num_; i++)
	{
		edh->vertex[i] = routing_table[i];
	}

	iph->saddr() = my_id_;
	iph->daddr() = -1;
	iph->ttl_ 	 = 3 * IP_DEF_TTL;
}

void
EHDSAgent::recvData(Packet* p)
{
	struct hdr_cmn*				cmh = HDR_CMN(p);
	struct hdr_ip*				iph = HDR_IP(p);
	struct hdr_ehds_data* 	edh = HDR_EHDS_DATA(p);

	if (cmh->direction() == hdr_cmn::UP	&& edh->daddr_ == my_id_)	// up to destination
	{
		dumpHopcount(p);
		port_dmux_->recv(p, 0);
		return;
	}
	else
	{
		// meet hole
		if (circleHole_list_ && iph->saddr() != my_id_ && edh->type_ == EHDS_DATA_GREEDY)
		{
			if (storage_opt_ == STORAGE_ALL)
			{
				sendHA(iph->saddr(), edh->vertex[edh->vertex_num_]);
			}
			edh->type_ = EHDS_DATA_ROUTING;
		}

		// -------- forward by greedy

		node * nexthop = NULL;
		while (nexthop == NULL && edh->vertex_num_ > 0)
		{
			nexthop = getNeighborByGreedy(edh->vertex[edh->vertex_num_ - 1]);
			if (nexthop == NULL)
				edh->vertex_num_--;
		}

		if (nexthop == NULL)	// no neighbor close
		{
			drop(p, DROP_RTR_NO_ROUTE);
			return;
		}
		else
		{
			cmh->direction() = hdr_cmn::DOWN;
			cmh->addr_type() = NS_AF_INET;
			cmh->last_hop_ = my_id_;
			cmh->next_hop_ = nexthop->id_;
			send(p, 0);
		}
	}
}

// --------------------- dump ------------------------------------- //

void
EHDSAgent::dumpApproximateHole()
{
	FILE *fp = fopen("ApproximateHole.tr", "a+");
	for	(struct circleHole* h = circleHole_list_; h; h = h->next_)
	{
		fprintf(fp, "%f\t%f\t%f\n", h->x_, h->y_, h->radius_);
	}
	fclose(fp);
}

void
EHDSAgent::dumpRoutingTable()
{
	if (routing_num_ > 1)
	{
		FILE* fp = fopen("RoutingTable.tr", "a+");
		for	(int i = routing_num_; i >= 0; i--)
		{
			fprintf(fp, "%d\t%f\t%f\n", my_id_ ,routing_table[i].x_, routing_table[i].y_);
		}
		fprintf(fp, "\n");

		fclose(fp);
	}
}

void
EHDSAgent::dumpHopcount(Packet* p)
{
	hdr_cmn * cmh = HDR_CMN(p);

	FILE *fp = fopen("Hopcount.tr", "a+");

	fprintf(fp, "%d	%d	%d\n", my_id_, cmh->uid(), cmh->num_forwards_);

	fclose(fp);
}

void
EHDSAgent::dumpBroadcastRegion()
{
	FILE *fp = fopen("BroadcastRegion.tr", "a+");
	fprintf(fp, "%d %f %f\n", my_id_, x_, y_);
	fclose(fp);
}