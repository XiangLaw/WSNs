/*
 * scalehexagon.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */

#include "../include/tcl.h"
#include "scalehexagon.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

static class ScaleHexagonAgentClass : public TclClass {
public:
	ScaleHexagonAgentClass() : TclClass("Agent/SCALEHEXAGON") {}
	TclObject* create(int, const char*const*) {
		return (new ScaleHexagonAgent());
	}
}class_scalehexagon;

// --------------------- Agent ------------------------------------ //

ScaleHexagonAgent::ScaleHexagonAgent() : BoundHoleAgent()
{
	circleHole_list_ = NULL;

	// clear trace file
	FILE * fp;
	fp = fopen("ApproximateHole.tr", "w");	fclose(fp);
	fp = fopen("RoutingTable.tr", "w");		fclose(fp);
	fp = fopen("Hopcount.tr", "w");			fclose(fp);
}

void
ScaleHexagonAgent::recv(Packet* p, Handler* h)
{
	struct hdr_cmn* cmh = HDR_CMN(p);

	if (cmh->ptype() == PT_HEXAGON)
		recvScaleHexagon(p);
	else
		BoundHoleAgent::recv(p, h);
}

int
ScaleHexagonAgent::command(int argc, const char*const* argv)
{
	return BoundHoleAgent::command(argc, argv);
}

// --------------------- Approximate hole ------------------------- //

void
ScaleHexagonAgent::createHole(Packet* p)
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

	// dump
	dumpApproximateHole();
}

// --------------------- Broadcast Hole identifier ---------------- //

void
ScaleHexagonAgent::sendScaleHexagon(nsaddr_t saddr, Point spos)
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

	recvScaleHexagon(p);
}

void
ScaleHexagonAgent::recvScaleHexagon(Packet* p)
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
ScaleHexagonAgent::routing(Packet* p)
{
	hdr_hexagon_data * hhd = HDR_HEXAGON_DATA(p);

	// clear routing table
	hhd->vertex_num_ = 0;

	if (dest->x_ || dest->y_)
	{
		// Add destination to routing table
		addrouting(dest, hhd);

		// create routing table
		Line SD = G::line(this, dest);

		for (circleHole* h = circleHole_list_; h; h = h->next_)
		{
			// check if need to routing to avoid this hole
			if (G::distance(h, SD) < h->radius_ && G::distance(this, h) >= h->radius_ && G::distance(dest, h) >= h->radius_)
			{
				// extend circle
				Circle c;
				bool ok = true;
				double m, ci;
				while (ok)
				{
					c.x_ = fmod(rand(), h->radius_ * 2) + h->x_ - h->radius_;
					c.y_ = fmod(rand(), h->radius_ * 2) + h->y_ - h->radius_;

					ci = G::distance(c, h);
					m = MIN(G::distance(c, this), G::distance(c, dest));
					ok = ci > h->radius_ || (h->radius_ + ci) > m;
				}
				//c.radius_ = fmod(rand(), m - h->radius_ - ci) + h->radius_ + ci;
				c.radius_ = fmod(rand(), m);

				// code
				Angle alpha = G::angle(SD);
				double cosa = cos(alpha);
				double sina = sin(alpha);

				// rotate
				Point I;
				I.x_ = c.x_ * cosa + c.y_ * sina;
				I.y_ = c.y_ * cosa - c.x_ * sina;

				Point H1, H2, H3, H4;
				H1.x_ = I.x_ - c.radius_ / sqrt(3);	H1.y_ = I.y_ + c.radius_;
				H2.x_ = I.x_ + c.radius_ / sqrt(3);	H2.y_ = I.y_ + c.radius_;
				H3.x_ = I.x_ - c.radius_ / sqrt(3);	H3.y_ = I.y_ - c.radius_;
				H4.x_ = I.x_ + c.radius_ / sqrt(3);	H4.y_ = I.y_ - c.radius_;

				Point V1, V2, V3, V4;
				V1.x_ = H1.x_ * cosa - H1.y_ * sina;	V1.y_ = H1.y_ * cosa + H1.x_ * sina;
				V2.x_ = H2.x_ * cosa - H2.y_ * sina;	V2.y_ = H2.y_ * cosa + H2.x_ * sina;
				V3.x_ = H3.x_ * cosa - H3.y_ * sina;	V3.y_ = H3.y_ * cosa + H3.x_ * sina;
				V4.x_ = H4.x_ * cosa - H4.y_ * sina;	V4.y_ = H4.y_ * cosa + H4.x_ * sina;

				if (G::position(&V1, SD) == G::position(h, SD))
				{
					if (G::distance(this, &V3) > G::distance(this, &V4))
					{
						addrouting(&V3, hhd);
						addrouting(&V4, hhd);
					}
					else
					{
						addrouting(&V4, hhd);
						addrouting(&V3, hhd);
					}
				}
				else
				{
					if (G::distance(this, &V1) > G::distance(this, &V2))
					{
						addrouting(&V1, hhd);
						addrouting(&V2, hhd);
					}
					else
					{
						addrouting(&V2, hhd);
						addrouting(&V1, hhd);
					}
				}
			} // if (d < r)
		} // for each hole

		//free(SD);
	} // if (des->x_ || des->y_)

	// Add source to routing table
	addrouting(this, hhd);
	hhd->vertex_num_--;			// go to myself, remove "this" from routing table
}

void
ScaleHexagonAgent::addrouting(Point* point, hdr_hexagon_data * hhd)
{
	for (int i = 0; i < hhd->vertex_num_; i++)
	{
		if (hhd->vertex[i].x_ == point->x_ && hhd->vertex[i].y_ == point->y_)
			return;
	}

	hhd->vertex[hhd->vertex_num_].x_ = point->x_;
	hhd->vertex[hhd->vertex_num_].y_ = point->y_;
	hhd->vertex_num_++;
}

// --------------------- Send data -------------------------------- //

void
ScaleHexagonAgent::sendData(Packet* p)
{
	hdr_cmn*			cmh = HDR_CMN(p);
	hdr_ip*				iph = HDR_IP(p);
	hdr_hexagon_data* 	edh = HDR_HEXAGON_DATA(p);

	cmh->size() += IP_HDR_LEN + edh->size();
	cmh->direction_ = hdr_cmn::DOWN;

	edh->daddr_ = iph->daddr();
	if (circleHole_list_ != NULL)
	{
		edh->type_ 	=  HEXAGON_DATA_ROUTING;
		routing(p);
	}
	else
	{
		edh->type_ 	=  HEXAGON_DATA_GREEDY;
		edh->vertex_num_ = 1;
		edh->vertex[0] = *(this->dest);
		edh->vertex[1] = *(this);
	}

	iph->saddr() = my_id_;
	iph->daddr() = -1;
	iph->ttl_ 	 = 3 * IP_DEF_TTL;
}

void
ScaleHexagonAgent::recvData(Packet* p)
{
	struct hdr_cmn*				cmh = HDR_CMN(p);
	struct hdr_ip*				iph = HDR_IP(p);
	struct hdr_hexagon_data* 	edh = HDR_HEXAGON_DATA(p);

	if (cmh->direction() == hdr_cmn::UP	&& edh->daddr_ == my_id_)	// up to destination
	{
		port_dmux_->recv(p, 0);
		return;
	}
	else
	{
		// meet hole
		if (circleHole_list_ && iph->saddr() != my_id_ && edh->type_ == HEXAGON_DATA_GREEDY)
		{
			sendScaleHexagon(iph->saddr(), edh->vertex[edh->vertex_num_]);
			edh->type_ = HEXAGON_DATA_ROUTING;
			routing(p);
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
ScaleHexagonAgent::dumpApproximateHole()
{
	FILE *fp = fopen("ApproximateHole.tr", "a+");
	for	(struct circleHole* h = circleHole_list_; h; h = h->next_)
	{
		fprintf(fp, "%f\t%f\t%f\n", h->x_, h->y_, h->radius_);
	}
	fclose(fp);
}

