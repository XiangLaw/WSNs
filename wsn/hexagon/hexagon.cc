/*
 * hexagon.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */

#include "../include/tcl.h"
#include "hexagon.h"

int hdr_hexagon::offset_;

static class HexagonHeaderClass : public PacketHeaderClass {
public:
	HexagonHeaderClass() : PacketHeaderClass("PacketHeader/HEXAGON", sizeof(hdr_all_hexagon))
	{
		bind_offset(&hdr_hexagon::offset_);
	}
} class_hexagonhdr;

static class HexagonAgentClass : public TclClass {
public:
	HexagonAgentClass() : TclClass("Agent/HEXAGON") {}
	TclObject* create(int, const char*const*) {
		return (new HexagonAgent());
	}
}class_hexagon;

// --------------------- Agent ------------------------------------ //

HexagonAgent::HexagonAgent() : BoundHoleAgent()
{
	circleHole_list_ = NULL;
	routing_num_ = 0;

	// clear trace file
	FILE * fp;
	fp = fopen("ApproximateHole.tr", "w");	fclose(fp);
	fp = fopen("RoutingTable.tr", "w");		fclose(fp);
	fp = fopen("Hopcount.tr", "w");			fclose(fp);
}

void
HexagonAgent::recv(Packet* p, Handler* h)
{
	struct hdr_cmn* cmh = HDR_CMN(p);

	if (cmh->ptype() == PT_HEXAGON)
		recvHexagon(p);
	else
		BoundHoleAgent::recv(p, h);
}

int
HexagonAgent::command(int argc, const char*const* argv)
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
HexagonAgent::createHole(Packet* p)
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

	// dump
	dumpApproximateHole();
}

// --------------------- Broadcast Hole identifier ---------------- //

void
HexagonAgent::sendHexagon(nsaddr_t saddr, Point spos)
{
	Packet*	p = allocpkt();
	struct hdr_cmn*			cmh = HDR_CMN(p);
	struct hdr_ip*			iph = HDR_IP(p);
	struct hdr_hexagon_ha*	ehh = HDR_HEXAGON_HA(p);

	cmh->ptype() = PT_HEXAGON;
	cmh->size()	 = IP_HDR_LEN + ehh->size();

	iph->daddr() = saddr;
	iph->saddr() = my_id_;
	iph->sport() = RT_PORT;
	iph->dport() = RT_PORT;
	iph->ttl_	 = IP_DEF_TTL;

	ehh->id_	 			= circleHole_list_->hole_id_;
	ehh->circle_.x_ 		= circleHole_list_->x_;
	ehh->circle_.y_ 		= circleHole_list_->y_;
	ehh->circle_.radius_ 	= circleHole_list_->radius_;
	ehh->spos_				= spos;

	recvHexagon(p);
}

void
HexagonAgent::recvHexagon(Packet* p)
{
	struct hdr_cmn*			cmh = HDR_CMN(p);
	struct hdr_ip*			iph = HDR_IP(p);
	struct hdr_hexagon_ha*	edh = HDR_HEXAGON_HA(p);

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

	if (cmh->direction() == hdr_cmn::UP	&& iph->daddr() == my_id_)	// up to destination
	{
		drop(p, "Received");
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

// --------------------- Routing ---------------------------------- //

void
HexagonAgent::routing()
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
				// code
				Angle alpha = G::angle(SD);
				double cosa = cos(alpha);
				double sina = sin(alpha);

				// rotage
				Point I;
				I.x_ = h->x_ * cosa + h->y_ * sina;
				I.y_ = h->y_ * cosa - h->x_ * sina;

				Point H1, H2, H3, H4;
				H1.x_ = I.x_ - h->radius_ / sqrt(3);	H1.y_ = I.y_ + h->radius_;
				H2.x_ = I.x_ + h->radius_ / sqrt(3);	H2.y_ = I.y_ + h->radius_;
				H3.x_ = I.x_ - h->radius_ / sqrt(3);	H3.y_ = I.y_ - h->radius_;
				H4.x_ = I.x_ + h->radius_ / sqrt(3);	H4.y_ = I.y_ - h->radius_;

				Point V1, V2, V3, V4;
				V1.x_ = H1.x_ * cosa - H1.y_ * sina;	V1.y_ = H1.y_ * cosa + H1.x_ * sina;
				V2.x_ = H2.x_ * cosa - H2.y_ * sina;	V2.y_ = H2.y_ * cosa + H2.x_ * sina;
				V3.x_ = H3.x_ * cosa - H3.y_ * sina;	V3.y_ = H3.y_ * cosa + H3.x_ * sina;
				V4.x_ = H4.x_ * cosa - H4.y_ * sina;	V4.y_ = H4.y_ * cosa + H4.x_ * sina;

				if (G::position(&V1, SD) == G::position(h, SD))
				{
					if (G::distance(this, &V3) > G::distance(this, &V4))
					{
						addrouting(&V3);
						addrouting(&V4);
					}
					else
					{
						addrouting(&V4);
						addrouting(&V3);
					}
				}
				else
				{
					if (G::distance(this, &V1) > G::distance(this, &V2))
					{
						addrouting(&V1);
						addrouting(&V2);
					}
					else
					{
						addrouting(&V2);
						addrouting(&V1);
					}
				}
			} // if (d < r)
		} // for each hole

		//free(SD);
	} // if (des->x_ || des->y_)

	// Add source to routing table
	addrouting(this);
	routing_num_--;			// go to myself, remove "this" from routing table
}

void
HexagonAgent::addrouting(Point* p)
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
HexagonAgent::sendData(Packet* p)
{
	hdr_cmn*			cmh = HDR_CMN(p);
	hdr_ip*				iph = HDR_IP(p);
	hdr_hexagon_data* 	edh = HDR_HEXAGON_DATA(p);

	cmh->size() += IP_HDR_LEN + edh->size();
	cmh->direction_ = hdr_cmn::DOWN;

	edh->type_ 			= routing_num_ > 2 ? HEXAGON_DATA_ROUTING : HEXAGON_DATA_GREEDY;
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
HexagonAgent::recvData(Packet* p)
{
	struct hdr_cmn*				cmh = HDR_CMN(p);
	struct hdr_ip*				iph = HDR_IP(p);
	struct hdr_hexagon_data* 	edh = HDR_HEXAGON_DATA(p);

	if (cmh->direction() == hdr_cmn::UP	&& edh->daddr_ == my_id_)	// up to destination
	{
		dumpHopcount(p);
		port_dmux_->recv(p, 0);
		return;
	}
	else
	{
		// meet hole
		if (circleHole_list_ && iph->saddr() != my_id_ && edh->type_ == HEXAGON_DATA_GREEDY)
		{
			sendHexagon(iph->saddr(), edh->vertex[edh->vertex_num_]);
			edh->type_ = HEXAGON_DATA_ROUTING;
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
HexagonAgent::dumpApproximateHole()
{
	FILE *fp = fopen("ApproximateHole.tr", "a+");
	for	(struct circleHole* h = circleHole_list_; h; h = h->next_)
	{
		fprintf(fp, "%f\t%f\t%f\n", h->x_, h->y_, h->radius_);
	}
	fclose(fp);
}

void
HexagonAgent::dumpRoutingTable()
{
	FILE* fp = fopen("RoutingTable.tr", "a+");
	for	(int i = routing_num_; i >= 0; i--)
	{
		fprintf(fp, "%d\t%f\t%f\n", my_id_ ,routing_table[i].x_, routing_table[i].y_);
	}
	fprintf(fp, "\n");

	fclose(fp);
}
